// F5 · S6 블루투스: A2DP 수신 → 버퍼 → 별도 출력 작업 → I2S DAC(PCM5102A).
// BT 콜백은 버퍼에 넣기만 하고 바로 돌아간다(DAC를 기다리면 BT 수신이 밀려 데이터를 버림).
#include <Arduino.h>
#include <freertos/stream_buffer.h>
#include "AudioTools.h"
#include <BluetoothA2DPSink.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");
static_assert(TFT_MOSI == PIN_SPI_MOSI && TFT_SCLK == PIN_SPI_SCK && TFT_MISO == PIN_SPI_MISO,
              "platformio.ini SPI 핀이 pins.h와 다름");

constexpr const char *BT_NAME = "PocketHiFi";
constexpr uint8_t ROTATION_PORTRAIT = 0;
constexpr uint32_t RATE_WINDOW_MS = 1000;
constexpr uint32_t SCREEN_REFRESH_MS = 1000;  // 하단 숫자 갱신 주기(사용자 요청 1초)
constexpr size_t META_LEN = 128;
constexpr int16_t LINE_HEIGHT = 20;
constexpr uint8_t START_VOLUME = 13;  // 0~127 중 약 0.1. 이후 폰 볼륨 버튼으로 조절
constexpr int MAX_VOLUME = 127;
constexpr uint16_t BT_START_SAMPLE_RATE = 44100;
constexpr int I2S_DMA_BUFFER_COUNT = 16;
constexpr int I2S_DMA_BUFFER_SIZE = 512;
constexpr size_t AUDIO_BUF_BYTES = 32 * 1024;  // 약 0.19초 분량: 폰이 몰아서 보내는 사이 공백(0.1~0.2초)을 버팀
constexpr size_t PRIME_BYTES = 20 * 1024;      // 시작·끊김 뒤 이만큼 모이면 출력 시작
constexpr size_t WRITE_CHUNK_BYTES = 1024;
constexpr uint32_t WRITER_STACK = 4096;
constexpr UBaseType_t WRITER_PRIORITY = configMAX_PRIORITIES - 5;
constexpr BaseType_t WRITER_CORE = 1;
constexpr uint32_t GOOD_KB_PER_SEC = 168;
// DMA 출력 버퍼가 담는 시간(16 × 512 프레임 ÷ 44.1 kHz ≈ 186 ms). 쓰기 사이 공백이 이보다 길면 실제로 소리가 끊긴다.
// 중간 버퍼가 잠깐 비는 것은 DMA가 메워 주므로 끊김이 아니다(S6 30분 테스트에서 확인).
constexpr uint32_t DMA_HOLD_MS = I2S_DMA_BUFFER_COUNT * I2S_DMA_BUFFER_SIZE * 1000UL / BT_START_SAMPLE_RATE;

// 블루투스 콜백은 BT 태스크에서 불린다. 여기서는 값만 복사하고 화면은 loop()에서 그린다.
struct NowPlaying {
  char title[META_LEN] = "";
  char artist[META_LEN] = "";
  char album[META_LEN] = "";
  const char *connection = "대기 중";
  const char *playback = "-";
  bool playing = false;
  bool dirty = true;
};

I2SStream i2s;
BluetoothA2DPSink a2dp;
TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
NowPlaying now;
portMUX_TYPE nowLock = portMUX_INITIALIZER_UNLOCKED;
StreamBufferHandle_t audioBuf = nullptr;
volatile uint32_t bytesReceived = 0;
volatile uint32_t droppedBytes = 0;  // 버퍼가 꽉 차 버린 양(출력이 못 따라감)
volatile uint32_t dropouts = 0;      // 실제로 소리가 끊긴 횟수(DMA 출력 버퍼까지 바닥남)
volatile uint16_t pendingSampleRate = 0;

void copyMeta(char *dst, const uint8_t *src) {
  strncpy(dst, reinterpret_cast<const char *>(src), META_LEN - 1);
  dst[META_LEN - 1] = '\0';
}

void onMetadata(uint8_t id, const uint8_t *text) {
  Serial.printf("[F5] AVRCP 0x%02x: %s\n", id, text);
  portENTER_CRITICAL(&nowLock);
  if (id == ESP_AVRC_MD_ATTR_TITLE) copyMeta(now.title, text);
  if (id == ESP_AVRC_MD_ATTR_ARTIST) copyMeta(now.artist, text);
  if (id == ESP_AVRC_MD_ATTR_ALBUM) copyMeta(now.album, text);
  now.dirty = true;
  portEXIT_CRITICAL(&nowLock);
}

void onConnection(esp_a2d_connection_state_t state, void *) {
  const char *label = state == ESP_A2D_CONNECTION_STATE_CONNECTED ? "연결됨"
                      : state == ESP_A2D_CONNECTION_STATE_CONNECTING ? "연결 중"
                                                                      : "대기 중";
  Serial.printf("[F5] 연결 상태: %s\n", a2dp.to_str(state));
  portENTER_CRITICAL(&nowLock);
  now.connection = label;
  now.dirty = true;
  portEXIT_CRITICAL(&nowLock);
}

void onPlayStatus(esp_avrc_playback_stat_t status) {
  const char *label = status == ESP_AVRC_PLAYBACK_PLAYING ? "재생"
                      : status == ESP_AVRC_PLAYBACK_PAUSED ? "일시정지"
                      : status == ESP_AVRC_PLAYBACK_STOPPED ? "정지"
                                                            : "-";
  Serial.printf("[F5] 재생 상태: %s\n", label);
  portENTER_CRITICAL(&nowLock);
  now.playback = label;
  now.playing = status == ESP_AVRC_PLAYBACK_PLAYING;
  now.dirty = true;
  portEXIT_CRITICAL(&nowLock);
}

void onSampleRate(uint16_t rate) {
  pendingSampleRate = rate;  // 출력 작업에서 I2S에 반영
}

// BT 콜백: 절대 기다리지 않는다. 자리가 없으면 패킷 전체를 버려 프레임 정렬을 지킨다.
void onAudioData(const uint8_t *data, uint32_t length) {
  bytesReceived += length;
  if (xStreamBufferSpacesAvailable(audioBuf) < length) {
    droppedBytes += length;
    return;
  }
  xStreamBufferSend(audioBuf, data, length, 0);
}

void applyVolume(int16_t *samples, size_t count, int volume) {
  if (volume >= MAX_VOLUME) return;
  for (size_t i = 0; i < count; ++i) {
    samples[i] = static_cast<int16_t>(static_cast<int32_t>(samples[i]) * volume / MAX_VOLUME);
  }
}

void applyPendingSampleRate() {
  const uint16_t rate = pendingSampleRate;
  if (rate == 0) return;
  pendingSampleRate = 0;
  AudioInfo info = i2s.audioInfo();
  if (info.sample_rate != rate) {
    info.sample_rate = rate;
    i2s.setAudioInfo(info);
  }
  Serial.printf("[F5] 샘플레이트 %u Hz\n", rate);
}

// 출력 작업: 버퍼에서 꺼내 볼륨을 적용하고 DAC로 보낸다. 비면 다시 채워질 때까지 기다린다.
void audioWriterTask(void *) {
  static uint8_t chunk[WRITE_CHUNK_BYTES];
  bool primed = false;
  uint32_t lastWriteMs = 0;
  for (;;) {
    applyPendingSampleRate();
    if (!primed) {
      if (xStreamBufferBytesAvailable(audioBuf) < PRIME_BYTES) {
        vTaskDelay(pdMS_TO_TICKS(5));
        continue;
      }
      primed = true;
    }
    const size_t n = xStreamBufferReceive(audioBuf, chunk, sizeof(chunk), pdMS_TO_TICKS(30));
    if (n == 0) {
      primed = false;  // 중간 버퍼가 잠깐 빔: DMA에 남은 소리로 버티는 동안 다시 모은다
      continue;
    }
    const uint32_t nowMs = millis();
    if (now.playing && lastWriteMs != 0 && nowMs - lastWriteMs > DMA_HOLD_MS) ++dropouts;
    lastWriteMs = nowMs;
    applyVolume(reinterpret_cast<int16_t *>(chunk), n / sizeof(int16_t), a2dp.get_volume());
    i2s.write(chunk, n);
  }
}

void drawLine(int16_t y, const char *label, const char *value, uint16_t color) {
  u8f.setForegroundColor(TFT_DARKGREY);
  u8f.setCursor(6, y);
  u8f.print(label);
  u8f.setForegroundColor(color);
  u8f.setCursor(6, y + LINE_HEIGHT);
  u8f.print(value);
}

void drawNowPlaying(const NowPlaying &snap) {
  tft.fillRect(0, 0, tft.width(), 252, TFT_BLACK);
  u8f.setForegroundColor(TFT_YELLOW);
  u8f.setCursor(6, 20);
  u8f.print(String("블루투스 · ") + snap.connection);
  drawLine(56, "제목", snap.title, TFT_WHITE);
  drawLine(104, "아티스트", snap.artist, TFT_CYAN);
  drawLine(152, "앨범", snap.album, TFT_WHITE);
  drawLine(200, "상태", snap.playback, TFT_GREEN);
}

void drawStats(uint32_t kbPerSec, uint32_t droppedKb, uint32_t dropoutCount, uint32_t playSeconds) {
  tft.fillRect(0, 254, tft.width(), 66, TFT_BLACK);
  u8f.setForegroundColor(kbPerSec >= GOOD_KB_PER_SEC ? TFT_GREEN : TFT_ORANGE);
  u8f.setCursor(6, 272);
  u8f.print(String("수신 ") + kbPerSec + " KB/s · 볼륨 " + a2dp.get_volume());
  u8f.setForegroundColor(droppedKb + dropoutCount == 0 ? TFT_GREEN : TFT_ORANGE);
  u8f.setCursor(6, 294);
  u8f.print(String("넘침 ") + droppedKb + "KB · 끊김 " + dropoutCount);
  u8f.setForegroundColor(TFT_CYAN);
  u8f.setCursor(6, 316);
  u8f.print(String("재생 ") + playSeconds / 60 + "분 " + playSeconds % 60 + "초");
}

void beginDac() {
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck = PIN_DAC_BCK;
  cfg.pin_ws = PIN_DAC_LRCK;
  cfg.pin_data = PIN_DAC_DATA;
  cfg.sample_rate = BT_START_SAMPLE_RATE;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  cfg.buffer_count = I2S_DMA_BUFFER_COUNT;
  cfg.buffer_size = I2S_DMA_BUFFER_SIZE;
  i2s.begin(cfg);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F5 bluetooth -> buffer -> DAC");

  pinMode(PIN_SD_CS, OUTPUT);  // SD는 쓰지 않지만 버스에 끼어들지 않게 비선택
  digitalWrite(PIN_SD_CS, HIGH);
  tft.init();
  tft.setRotation(ROTATION_PORTRAIT);
  tft.fillScreen(TFT_BLACK);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);

  audioBuf = xStreamBufferCreate(AUDIO_BUF_BYTES, 1);
  beginDac();
  xTaskCreatePinnedToCore(audioWriterTask, "audioOut", WRITER_STACK, nullptr, WRITER_PRIORITY,
                          nullptr, WRITER_CORE);

  a2dp.set_stream_reader(onAudioData, false);  // 라이브러리 I2S 출력은 끄고 직접 출력
  a2dp.set_sample_rate_callback(onSampleRate);
  a2dp.set_on_connection_state_changed(onConnection);
  a2dp.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST |
                                        ESP_AVRC_MD_ATTR_ALBUM);
  a2dp.set_avrc_metadata_callback(onMetadata);
  a2dp.set_avrc_rn_playstatus_callback(onPlayStatus);
  a2dp.start(BT_NAME);
  a2dp.set_volume(START_VOLUME);
  Serial.printf("[F5] '%s' 광고 시작, free heap %u\n", BT_NAME, ESP.getFreeHeap());
  drawStats(0, 0, 0, 0);
}

void loop() {
  static uint32_t lastRateMs = 0;
  static uint32_t lastScreenMs = 0;
  static uint32_t lastKb = 0;
  static uint32_t playSeconds = 0;
  if (now.dirty) {
    portENTER_CRITICAL(&nowLock);
    NowPlaying snap = now;
    now.dirty = false;
    portEXIT_CRITICAL(&nowLock);
    drawNowPlaying(snap);
  }
  if (millis() - lastRateMs >= RATE_WINDOW_MS) {
    lastRateMs = millis();
    lastKb = bytesReceived / 1024;
    bytesReceived = 0;
    if (now.playing) ++playSeconds;
    if (now.playing && playSeconds % 60 == 0) {
      Serial.printf("[F5] 재생 %u분, %u KB/s, 넘침 %u KB, 끊김 %u, heap %u\n", playSeconds / 60, lastKb,
                    droppedBytes / 1024, dropouts, ESP.getFreeHeap());
    }
  }
  if (millis() - lastScreenMs >= SCREEN_REFRESH_MS) {
    lastScreenMs = millis();
    drawStats(lastKb, droppedBytes / 1024, dropouts, playSeconds);
  }
  delay(5);
}
