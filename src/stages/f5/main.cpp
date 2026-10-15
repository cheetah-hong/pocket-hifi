// F5 · S6 블루투스: A2DP 수신 → I2S DAC(PCM5102A) 출력 + AVRCP 곡 정보·끊김 감시를 화면에 표시.
#include <Arduino.h>
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
constexpr uint32_t SCREEN_REFRESH_MS = 5000;  // 재생 중 하단 숫자 갱신 주기(화면 그리기 부담 줄임)
constexpr uint32_t GAP_MS = 80;               // 소리 데이터가 이보다 오래 안 오면 수신 끊김
constexpr size_t META_LEN = 128;
constexpr int16_t LINE_HEIGHT = 20;
constexpr uint8_t START_VOLUME = 13;       // 0~127 중 약 0.1. 이후 폰 볼륨 버튼으로 조절
constexpr int I2S_DMA_BUFFER_COUNT = 16;  // 출력 여유: 기본보다 크게
constexpr int I2S_DMA_BUFFER_SIZE = 512;

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
BluetoothA2DPSink a2dp(i2s);  // 큐 방식은 링 버퍼가 메모리를 많이 써(여유 48 KB) 수신량이 떨어져서 기본 방식 사용
TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
NowPlaying now;
portMUX_TYPE nowLock = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t bytesReceived = 0;
volatile uint32_t lastDataMs = 0;

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

void onAudioData(const uint8_t *, uint32_t length) {
  bytesReceived += length;  // 소리는 I2S로 나가고, 여기서는 양만 센다
  lastDataMs = millis();
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
  tft.fillRect(0, 0, tft.width(), 270, TFT_BLACK);
  u8f.setForegroundColor(TFT_YELLOW);
  u8f.setCursor(6, 20);
  u8f.print(String("블루투스 · ") + snap.connection);
  drawLine(56, "제목", snap.title, TFT_WHITE);
  drawLine(104, "아티스트", snap.artist, TFT_CYAN);
  drawLine(152, "앨범", snap.album, TFT_WHITE);
  drawLine(200, "상태", snap.playback, TFT_GREEN);
}

void drawRate(uint32_t kbPerSec, uint32_t stalls, uint32_t playSeconds) {
  tft.fillRect(0, 272, tft.width(), 48, TFT_BLACK);
  u8f.setForegroundColor(kbPerSec > 0 ? TFT_GREEN : TFT_DARKGREY);
  u8f.setCursor(6, 290);
  u8f.print(String("수신 ") + kbPerSec + " KB/s · 끊김 " + stalls);  // stalls = 수신 끊김 수
  u8f.setForegroundColor(TFT_CYAN);
  u8f.setCursor(6, 312);
  u8f.print(String("재생 ") + playSeconds / 60 + "분 " + playSeconds % 60 + "초");
}

void beginDac() {
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck = PIN_DAC_BCK;
  cfg.pin_ws = PIN_DAC_LRCK;
  cfg.pin_data = PIN_DAC_DATA;
  cfg.sample_rate = 44100;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  cfg.buffer_count = I2S_DMA_BUFFER_COUNT;
  cfg.buffer_size = I2S_DMA_BUFFER_SIZE;
  i2s.begin(cfg);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F5 bluetooth test");

  pinMode(PIN_SD_CS, OUTPUT);  // SD는 쓰지 않지만 버스에 끼어들지 않게 비선택
  digitalWrite(PIN_SD_CS, HIGH);
  tft.init();
  tft.setRotation(ROTATION_PORTRAIT);
  tft.fillScreen(TFT_BLACK);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);

  beginDac();
  a2dp.set_stream_reader(onAudioData, true);  // I2S 출력 켬 + 수신량 측정
  a2dp.set_on_connection_state_changed(onConnection);
  a2dp.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST |
                                        ESP_AVRC_MD_ATTR_ALBUM);
  a2dp.set_avrc_metadata_callback(onMetadata);
  a2dp.set_avrc_rn_playstatus_callback(onPlayStatus);
  a2dp.start(BT_NAME);
  a2dp.set_volume(START_VOLUME);
  Serial.printf("[F5] '%s' 광고 시작, free heap %u\n", BT_NAME, ESP.getFreeHeap());
  drawRate(0, 0, 0);
}

// 재생 중 소리 데이터가 GAP_MS 넘게 끊긴 구간을 세고 로그로 남긴다.
uint32_t checkInputGap(bool playing) {
  static bool inGap = false;
  static uint32_t gapStartMs = 0;
  static uint32_t gaps = 0;
  const uint32_t nowMs = millis();
  const uint32_t since = nowMs - lastDataMs;
  if (playing && !inGap && since > GAP_MS) {
    inGap = true;
    gapStartMs = lastDataMs;
  } else if (inGap && since <= GAP_MS) {
    inGap = false;
    ++gaps;
    Serial.printf("[F5] 수신 끊김 #%u: %u ms (t=%u s)\n", gaps, lastDataMs - gapStartMs, nowMs / 1000);
  }
  return gaps;
}

void loop() {
  static uint32_t lastScreenMs = 0;
  static uint32_t lastKb = 0;
  const uint32_t gaps = checkInputGap(now.playing);
  static uint32_t lastRateMs = 0;
  if (now.dirty) {
    portENTER_CRITICAL(&nowLock);
    NowPlaying snap = now;
    now.dirty = false;
    portEXIT_CRITICAL(&nowLock);
    drawNowPlaying(snap);
  }
  static uint32_t playSeconds = 0;
  if (millis() - lastRateMs >= RATE_WINDOW_MS) {
    lastRateMs = millis();
    lastKb = bytesReceived / 1024;
    bytesReceived = 0;
    if (now.playing) ++playSeconds;
    if (now.playing && playSeconds % 60 == 0) {
      Serial.printf("[F5] 재생 %u분, 수신 끊김 %u, %u KB/s, heap %u\n", playSeconds / 60, gaps, lastKb,
                    ESP.getFreeHeap());
    }
  }
  if (millis() - lastScreenMs >= SCREEN_REFRESH_MS) {
    lastScreenMs = millis();
    drawRate(lastKb, gaps, playSeconds);
  }
  delay(5);
}
