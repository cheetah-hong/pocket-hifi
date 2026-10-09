// F3 추가 · 앨범 커버: /music/<아티스트>/<앨범>/cover.jpg를 세로 화면 위쪽 240×240에 표시
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <U8g2_for_TFT_eSPI.h>
#include <vector>
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");
static_assert(TFT_MOSI == PIN_SPI_MOSI && TFT_SCLK == PIN_SPI_SCK && TFT_MISO == PIN_SPI_MISO,
              "platformio.ini SPI 핀이 pins.h와 다름");

constexpr uint32_t SD_FREQ_FAST_HZ = 20000000;
constexpr uint32_t SD_FREQ_SLOW_HZ = 4000000;
constexpr uint8_t SD_TRIES_PER_FREQ = 2;
constexpr const char *MUSIC_ROOT = "/music";
constexpr const char *COVER_NAME = "cover.jpg";
constexpr uint8_t ROTATION_PORTRAIT = 0;
constexpr int16_t COVER_SIZE = 240;
constexpr uint32_t ALBUM_HOLD_MS = 6000;

struct Album {
  String artist;
  String title;
  String dir;
  uint16_t trackCount;
};

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
std::vector<Album> albums;

bool isHidden(const char *name) {
  return name[0] == '.';
}

bool hasMp3Extension(const String &name) {
  String lower = name;
  lower.toLowerCase();
  return lower.endsWith(".mp3");
}

bool onJpegBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

uint16_t countTracks(File albumDir) {
  uint16_t count = 0;
  for (File f = albumDir.openNextFile(); f; f = albumDir.openNextFile()) {
    if (!f.isDirectory() && !isHidden(f.name()) && hasMp3Extension(f.name())) ++count;
    f.close();
  }
  return count;
}

void addAlbumsOfArtist(File artistDir) {
  for (File albumDir = artistDir.openNextFile(); albumDir; albumDir = artistDir.openNextFile()) {
    if (albumDir.isDirectory() && !isHidden(albumDir.name())) {
      albums.push_back({artistDir.name(), albumDir.name(), albumDir.path(), countTracks(albumDir)});
    }
    albumDir.close();
  }
}

void findAlbums() {
  File root = SD.open(MUSIC_ROOT);
  if (!root || !root.isDirectory()) return;
  for (File artistDir = root.openNextFile(); artistDir; artistDir = root.openNextFile()) {
    if (artistDir.isDirectory() && !isHidden(artistDir.name())) addAlbumsOfArtist(artistDir);
    artistDir.close();
  }
  root.close();
}

// 리셋 직후 첫 시도가 실패하는 경우가 있어 같은 속도로 한 번 더, 그다음 느린 속도로 시도한다.
bool mountSd(SPIClass &spi) {
  const uint32_t freqs[] = {SD_FREQ_FAST_HZ, SD_FREQ_SLOW_HZ};
  for (uint32_t freq : freqs) {
    for (uint8_t attempt = 1; attempt <= SD_TRIES_PER_FREQ; ++attempt) {
      if (SD.begin(PIN_SD_CS, spi, freq)) {
        Serial.printf("[F3C] SD mount OK @ %u MHz (시도 %u)\n", freq / 1000000, attempt);
        return true;
      }
      Serial.printf("[F3C] SD mount 실패 @ %u MHz (시도 %u)\n", freq / 1000000, attempt);
      SD.end();
      delay(100);
    }
  }
  return false;
}

void drawText(int16_t y, const String &text, uint16_t color) {
  u8f.setForegroundColor(color);
  u8f.setCursor(6, y);
  u8f.print(text);
}

void drawCoverPlaceholder() {
  tft.fillRect(0, 0, COVER_SIZE, COVER_SIZE, TFT_DARKGREY);
  drawText(COVER_SIZE / 2, "cover.jpg 없음", TFT_WHITE);
}

void showAlbum(const Album &album) {
  tft.fillScreen(TFT_BLACK);
  const String coverPath = album.dir + "/" + COVER_NAME;
  if (SD.exists(coverPath)) {
    const uint32_t start = millis();
    const JRESULT result = TJpgDec.drawFsJpg(0, 0, coverPath.c_str(), SD);
    Serial.printf("[F3C] %s → %s, %lu ms\n", coverPath.c_str(),
                  result == JDR_OK ? "OK" : "디코드 실패", millis() - start);
    if (result != JDR_OK) drawCoverPlaceholder();
  } else {
    Serial.printf("[F3C] %s 없음\n", coverPath.c_str());
    drawCoverPlaceholder();
  }
  drawText(COVER_SIZE + 22, album.title, TFT_WHITE);
  drawText(COVER_SIZE + 42, album.artist, TFT_CYAN);
  drawText(COVER_SIZE + 62, String(album.trackCount) + "곡", TFT_YELLOW);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F3 cover test");

  // 화면 초기화 중 SD 카드가 버스 신호를 받지 않도록 SD CS를 먼저 비선택(HIGH)으로 둔다.
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  tft.init();
  tft.setRotation(ROTATION_PORTRAIT);
  tft.setSwapBytes(true);  // TJpgDec 출력은 RGB565 바이트 순서가 반대
  tft.fillScreen(TFT_BLACK);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(onJpegBlock);

  if (!mountSd(tft.getSPIinstance())) {
    drawText(30, "SD mount 실패", TFT_RED);
    return;
  }
  findAlbums();
  Serial.printf("[F3C] 앨범 %u개\n", albums.size());
  for (const Album &a : albums) {
    Serial.printf("  %s / %s (%u곡)\n", a.artist.c_str(), a.title.c_str(), a.trackCount);
  }
}

void loop() {
  if (albums.empty()) {
    delay(ALBUM_HOLD_MS);
    return;
  }
  for (const Album &album : albums) {
    showAlbum(album);
    delay(ALBUM_HOLD_MS);
  }
}
