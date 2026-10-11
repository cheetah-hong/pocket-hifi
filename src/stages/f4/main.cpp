// F4 · S5 소리: SD의 mp3를 차례로 디코드해 I2S DAC(PCM5102A)로 재생. 볼륨은 0.1에서 시작.
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <vector>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");
static_assert(TFT_MOSI == PIN_SPI_MOSI && TFT_SCLK == PIN_SPI_SCK && TFT_MISO == PIN_SPI_MISO,
              "platformio.ini SPI 핀이 pins.h와 다름");

constexpr float START_VOLUME = 0.7f;  // DAC 잭 직결 기준(앰프는 V1로 연기). 0.1 → 0.3 → 0.7(사용자 확인)
constexpr size_t START_TRACK = 1;     // 0부터 셈: 1 = 2번 트랙
constexpr uint32_t SD_FREQ_HZ = 20000000;
constexpr const char *MUSIC_ROOT = "/music";
constexpr uint8_t ROTATION_LANDSCAPE = 1;
constexpr uint32_t STATUS_INTERVAL_MS = 5000;

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
I2SStream i2s;
VolumeStream volume(i2s);
EncodedAudioStream decoded(&volume, new MP3DecoderHelix());
StreamCopy copier;
std::vector<String> tracks;
File current;
size_t trackIndex = 0;

bool isHidden(const char *name) {
  return name[0] == '.';
}

bool hasMp3Extension(const String &name) {
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".mp3");
}

void collectTracks(File dir) {
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (isHidden(entry.name())) {
      // 건너뜀
    } else if (entry.isDirectory()) {
      collectTracks(entry);
    } else if (hasMp3Extension(entry.name())) {
      tracks.push_back(entry.path());
    }
    entry.close();
  }
}

void drawText(int16_t y, const String &text, uint16_t color) {
  u8f.setForegroundColor(color);
  u8f.setCursor(6, y);
  u8f.print(text);
}

String baseName(const String &path) {
  return path.substring(path.lastIndexOf('/') + 1);
}

void showTrack() {
  tft.fillScreen(TFT_BLACK);
  drawText(22, String("재생 중 ") + (trackIndex + 1) + "/" + tracks.size(), TFT_YELLOW);
  drawText(50, baseName(tracks[trackIndex]), TFT_WHITE);
  drawText(78, String("볼륨 ") + String(START_VOLUME, 2), TFT_CYAN);
}

bool openTrack(size_t index) {
  if (current) current.close();
  trackIndex = index % tracks.size();
  current = SD.open(tracks[trackIndex]);
  if (!current) {
    Serial.printf("[F4] 열기 실패: %s\n", tracks[trackIndex].c_str());
    return false;
  }
  decoded.begin();
  copier.begin(decoded, current);
  Serial.printf("[F4] ▶ %u/%u %s (%u KB)\n", trackIndex + 1, tracks.size(),
                tracks[trackIndex].c_str(), current.size() / 1024);
  showTrack();
  return true;
}

void beginAudio() {
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck = PIN_DAC_BCK;
  cfg.pin_ws = PIN_DAC_LRCK;
  cfg.pin_data = PIN_DAC_DATA;
  cfg.sample_rate = 44100;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  i2s.begin(cfg);
  volume.begin(cfg);
  volume.setVolume(START_VOLUME);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F4 SD mp3 playback");

  pinMode(PIN_SD_CS, OUTPUT);  // 화면 초기화 중 SD가 버스 신호를 받지 않게
  digitalWrite(PIN_SD_CS, HIGH);
  tft.init();
  tft.setRotation(ROTATION_LANDSCAPE);
  tft.fillScreen(TFT_BLACK);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);

  if (!SD.begin(PIN_SD_CS, tft.getSPIinstance(), SD_FREQ_HZ)) {
    Serial.println("[F4] SD mount 실패");
    drawText(30, "SD mount 실패", TFT_RED);
    return;
  }
  File root = SD.open(MUSIC_ROOT);
  if (root) collectTracks(root);
  Serial.printf("[F4] 곡 %u개, 볼륨 %.2f\n", tracks.size(), START_VOLUME);
  if (tracks.empty()) {
    drawText(30, "/music에 mp3 없음", TFT_RED);
    return;
  }
  beginAudio();
  openTrack(START_TRACK);
}

void loop() {
  static uint32_t lastStatusMs = 0;
  if (tracks.empty()) {
    delay(1000);
    return;
  }
  if (copier.copy() == 0 && current.available() == 0) {
    openTrack(trackIndex + 1);  // 곡이 끝나면 다음 곡
  }
  if (millis() - lastStatusMs >= STATUS_INTERVAL_MS) {
    lastStatusMs = millis();
    Serial.printf("[F4] %u/%u 진행 %u/%u KB, heap %u\n", trackIndex + 1, tracks.size(),
                  current.position() / 1024, current.size() / 1024, ESP.getFreeHeap());
  }
}
