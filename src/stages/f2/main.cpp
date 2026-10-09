// F2 · S3 화면 테스트: 색 채우기 → 영문·숫자 → 한글 → 세로 방향
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "pins.h"

// TFT_eSPI 핀은 platformio.ini build_flags에서 온다. pins.h와 다르면 빌드를 멈춘다.
static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");
static_assert(TFT_MOSI == PIN_SPI_MOSI && TFT_SCLK == PIN_SPI_SCK && TFT_MISO == PIN_SPI_MISO,
              "platformio.ini SPI 핀이 pins.h와 다름");

constexpr uint8_t ROTATION_LANDSCAPE = 1;
constexpr uint8_t ROTATION_PORTRAIT = 0;
constexpr uint32_t COLOR_HOLD_MS = 1000;
constexpr uint32_t STEP_HOLD_MS = 4000;

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;

void fillStep(const char *name, uint16_t color) {
  Serial.printf("[F2] 1 색 채우기: %s\n", name);
  tft.fillScreen(color);
  delay(COLOR_HOLD_MS);
}

void showAsciiStep() {
  Serial.println("[F2] 2 영문·숫자");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.drawString("PocketHiFi F2", 10, 10);
  tft.drawString("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 10, 40);
  tft.drawString("abcdefghijklmnopqrstuvwxyz", 10, 64);
  tft.drawString("0123456789  320x240", 10, 88);
  delay(STEP_HOLD_MS);
}

void drawKorean(int16_t x, int16_t y, const char *text, uint16_t color) {
  u8f.setForegroundColor(color);
  u8f.setCursor(x, y);
  u8f.print(text);
}

void showKoreanStep() {
  Serial.println("[F2] 3 한글: 안녕, 포켓 하이파이");
  tft.fillScreen(TFT_BLACK);
  drawKorean(10, 40, "안녕, 포켓 하이파이", TFT_WHITE);
  drawKorean(10, 70, "가로 방향 · 한글 글꼴", TFT_YELLOW);
  delay(STEP_HOLD_MS);
}

void showPortraitStep() {
  Serial.println("[F2] 4 세로 방향 (setRotation 0)");
  tft.setRotation(ROTATION_PORTRAIT);
  tft.fillScreen(TFT_NAVY);
  drawKorean(10, 30, "세로 방향", TFT_WHITE);
  drawKorean(10, 60, "위가 화면 위쪽이면", TFT_CYAN);
  drawKorean(10, 80, "정상입니다", TFT_CYAN);
  tft.fillRect(0, 0, 240, 6, TFT_RED);  // 위쪽 표시 막대
  delay(STEP_HOLD_MS);
  tft.setRotation(ROTATION_LANDSCAPE);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F2 display test");
  tft.init();
  tft.setRotation(ROTATION_LANDSCAPE);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);
  Serial.printf("[F2] tft %dx%d\n", tft.width(), tft.height());
}

void loop() {
  fillStep("빨강", TFT_RED);
  fillStep("초록", TFT_GREEN);
  fillStep("파랑", TFT_BLUE);
  showAsciiStep();
  showKoreanStep();
  showPortraitStep();
  Serial.println("[F2] 한 바퀴 끝, 다시 시작");
}
