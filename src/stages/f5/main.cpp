// F5 · S6 블루투스(1차): A2DP 수신 + AVRCP 곡 정보를 화면에 표시. DAC 전이라 소리는 내보내지 않는다.
#include <Arduino.h>
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
constexpr size_t META_LEN = 128;
constexpr int16_t LINE_HEIGHT = 20;

// 블루투스 콜백은 BT 태스크에서 불린다. 여기서는 값만 복사하고 화면은 loop()에서 그린다.
struct NowPlaying {
  char title[META_LEN] = "";
  char artist[META_LEN] = "";
  char album[META_LEN] = "";
  const char *connection = "대기 중";
  const char *playback = "-";
  bool dirty = true;
};

BluetoothA2DPSink a2dp;
TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
NowPlaying now;
portMUX_TYPE nowLock = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t bytesReceived = 0;

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
  now.dirty = true;
  portEXIT_CRITICAL(&nowLock);
}

void onAudioData(const uint8_t *, uint32_t length) {
  bytesReceived += length;  // DAC 전이라 버리고 양만 센다
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

void drawRate(uint32_t kbPerSec) {
  tft.fillRect(0, 280, tft.width(), 40, TFT_BLACK);
  u8f.setForegroundColor(kbPerSec > 0 ? TFT_GREEN : TFT_DARKGREY);
  u8f.setCursor(6, 304);
  u8f.print(String("수신 ") + kbPerSec + " KB/s");
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

  a2dp.set_stream_reader(onAudioData, false);  // I2S 출력 끔
  a2dp.set_on_connection_state_changed(onConnection);
  a2dp.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST |
                                        ESP_AVRC_MD_ATTR_ALBUM);
  a2dp.set_avrc_metadata_callback(onMetadata);
  a2dp.set_avrc_rn_playstatus_callback(onPlayStatus);
  a2dp.start(BT_NAME);
  Serial.printf("[F5] '%s' 광고 시작, free heap %u\n", BT_NAME, ESP.getFreeHeap());
  drawRate(0);
}

void loop() {
  static uint32_t lastRateMs = 0;
  if (now.dirty) {
    portENTER_CRITICAL(&nowLock);
    NowPlaying snap = now;
    now.dirty = false;
    portEXIT_CRITICAL(&nowLock);
    drawNowPlaying(snap);
  }
  if (millis() - lastRateMs >= RATE_WINDOW_MS) {
    lastRateMs = millis();
    const uint32_t kb = bytesReceived / 1024;
    bytesReceived = 0;
    drawRate(kb);
    if (kb > 0) Serial.printf("[F5] 수신 %u KB/s\n", kb);
  }
  delay(20);
}
