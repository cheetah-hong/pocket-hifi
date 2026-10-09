// F3 · S4 microSD: /music 아래 .mp3를 재귀로 찾아 시리얼과 화면에 목록 표시
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <vector>
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");
static_assert(TFT_MOSI == PIN_SPI_MOSI && TFT_SCLK == PIN_SPI_SCK && TFT_MISO == PIN_SPI_MISO,
              "platformio.ini SPI 핀이 pins.h와 다름");

constexpr uint32_t SD_FREQ_FAST_HZ = 20000000;
constexpr uint32_t SD_FREQ_SLOW_HZ = 4000000;
constexpr const char *MUSIC_ROOT = "/music";
constexpr uint8_t ROTATION_LANDSCAPE = 1;
constexpr int16_t LINE_HEIGHT = 18;
constexpr int16_t LIST_TOP = 40;

struct Track {
  String path;
  uint32_t sizeKb;
};

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;

bool hasMp3Extension(const String &name) {
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".mp3");
}

// macOS가 FAT 카드에 만드는 ._파일, .Spotlight 등 숨김 항목은 건너뛴다.
bool isHidden(const char *name) {
  return name[0] == '.';
}

void collectTracks(File dir, std::vector<Track> &tracks) {
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (isHidden(entry.name())) {
      // 건너뜀
    } else if (entry.isDirectory()) {
      collectTracks(entry, tracks);
    } else if (hasMp3Extension(entry.name())) {
      tracks.push_back({String(entry.path()), static_cast<uint32_t>(entry.size() / 1024)});
    }
    entry.close();
  }
}

bool mountSd(SPIClass &spi) {
  const uint32_t freqs[] = {SD_FREQ_FAST_HZ, SD_FREQ_SLOW_HZ};
  for (uint32_t freq : freqs) {
    if (SD.begin(PIN_SD_CS, spi, freq)) {
      Serial.printf("[F3] SD mount OK @ %u MHz, type %u, size %llu MB\n",
                    freq / 1000000, SD.cardType(), SD.cardSize() / (1024 * 1024));
      return true;
    }
    Serial.printf("[F3] SD mount 실패 @ %u MHz\n", freq / 1000000);
    SD.end();
  }
  return false;
}

void drawLine(int16_t y, const String &text, uint16_t color) {
  u8f.setForegroundColor(color);
  u8f.setCursor(6, y);
  u8f.print(text);
}

void showMessage(const char *title, const char *detail) {
  tft.fillScreen(TFT_BLACK);
  drawLine(30, title, TFT_YELLOW);
  drawLine(30 + LINE_HEIGHT, detail, TFT_WHITE);
}

void showTracks(const std::vector<Track> &tracks) {
  tft.fillScreen(TFT_BLACK);
  drawLine(22, String("곡 ") + tracks.size() + "개 · /music", TFT_YELLOW);
  const int16_t maxLines = (tft.height() - LIST_TOP) / LINE_HEIGHT + 1;
  const size_t rootLen = strlen(MUSIC_ROOT) + 1;
  for (size_t i = 0; i < tracks.size() && static_cast<int16_t>(i) < maxLines; ++i) {
    drawLine(LIST_TOP + i * LINE_HEIGHT, tracks[i].path.substring(rootLen), TFT_WHITE);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F3 microSD list");

  // 화면 먼저: TFT_eSPI가 SPI 버스를 18/19/23으로 시작한다.
  tft.init();
  tft.setRotation(ROTATION_LANDSCAPE);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);
  showMessage("SD 확인 중…", "");

  // TFT_eSPI는 전역 SPI가 아니라 자체 SPIClass(VSPI)를 쓴다. SD에 그 객체를 그대로 넘긴다.
  SPIClass &spi = tft.getSPIinstance();
  Serial.printf("[F3] SD는 화면의 SPI 인스턴스 사용 (tft.getSPIinstance @%p, 전역 SPI @%p)\n",
                static_cast<void *>(&spi), static_cast<void *>(&SPI));

  if (!mountSd(spi)) {
    showMessage("SD mount 실패", "CS 21 · 3.3V · FAT32 확인");
    return;
  }
  File root = SD.open(MUSIC_ROOT);
  if (!root || !root.isDirectory()) {
    Serial.println("[F3] /music 폴더 없음");
    showMessage("/music 폴더 없음", "카드 구조를 확인하세요");
    return;
  }
  std::vector<Track> tracks;
  collectTracks(root, tracks);
  root.close();

  Serial.printf("[F3] 곡 %u개\n", tracks.size());
  for (const Track &t : tracks) {
    Serial.printf("%s (%u KB)\n", t.path.c_str(), t.sizeKb);
  }
  showTracks(tracks);
  Serial.println("[F3] 화면에 목록 표시 완료");
}

void loop() {
  delay(2000);
  Serial.println("alive");
}
