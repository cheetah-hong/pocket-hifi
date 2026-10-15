// F6 · S7 버튼 입력 테스트: 위(32)·아래(33)·MENU(16)·PLAY(17). 누름·길게 누름을 세로 화면과 시리얼에 표시.
// V0은 엔코더 대신 32·33에 택트 버튼(HAS_ENCODER 0). 34번은 풀업이 없어 읽지 않는다.
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");

#define HAS_ENCODER 0

constexpr uint8_t ROTATION_PORTRAIT = 0;
constexpr uint32_t DEBOUNCE_MS = 30;     // 접점 떨림이 가라앉을 때까지 기다림
constexpr uint32_t LONG_PRESS_MS = 600;  // 이 이상 누르면 길게 누름
constexpr int16_t ROW_TOP = 150;
constexpr int16_t ROW_HEIGHT = 40;

struct Button {
  const char *name;
  int pin;
  uint16_t color;
  bool stablePressed;
  bool lastRead;
  uint32_t changedMs;
  uint32_t pressedMs;
  uint32_t presses;
  uint32_t longPresses;
};

Button buttons[] = {
    {"위", PIN_ENC_A, TFT_GREEN, false, false, 0, 0, 0, 0},
    {"아래", PIN_ENC_B, TFT_BLUE, false, false, 0, 0, 0, 0},
    {"MENU", PIN_BTN_MENU, TFT_ORANGE, false, false, 0, 0, 0, 0},
    {"PLAY", PIN_BTN_PLAY, TFT_RED, false, false, 0, 0, 0, 0},
};

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;

void drawText(int16_t x, int16_t y, const String &text, uint16_t color) {
  u8f.setForegroundColor(color);
  u8f.setCursor(x, y);
  u8f.print(text);
}

void drawHeader() {
  tft.fillScreen(TFT_BLACK);
  drawText(10, 24, "S7 버튼 테스트", TFT_YELLOW);
  drawText(10, 48, "버튼을 하나씩 눌러 보세요", TFT_WHITE);
  drawText(10, 68, "길게(0.6초) 누르기도 확인", TFT_DARKGREY);
}

void drawLastEvent(const Button &b, bool isLong) {
  tft.fillRect(0, 80, tft.width(), 56, TFT_BLACK);
  tft.fillRoundRect(10, 86, 220, 44, 8, b.color);
  u8f.setForegroundColor(TFT_BLACK);
  u8f.setBackgroundColor(b.color);
  u8f.setCursor(22, 114);
  u8f.print(String(b.name) + (isLong ? " 길게" : " 눌림"));
  u8f.setBackgroundColor(TFT_BLACK);
}

void drawCounts() {
  tft.fillRect(0, ROW_TOP - 18, tft.width(), ROW_HEIGHT * 4 + 10, TFT_BLACK);
  for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
    const Button &b = buttons[i];
    const int16_t y = ROW_TOP + i * ROW_HEIGHT;
    tft.fillCircle(18, y - 5, 7, b.stablePressed ? b.color : TFT_DARKGREY);
    drawText(34, y, String(b.name) + " (" + b.pin + ")", b.color);
    drawText(130, y, String(b.presses) + "회 · 길게 " + b.longPresses, TFT_WHITE);
  }
}

// 핀 상태가 DEBOUNCE_MS 동안 그대로일 때만 눌림/뗌으로 인정한다. 바뀌었으면 true.
bool updateButton(Button &b, uint32_t nowMs) {
  const bool reading = digitalRead(b.pin) == LOW;  // 내부 풀업: 누르면 LOW
  if (reading != b.lastRead) {
    b.lastRead = reading;
    b.changedMs = nowMs;
  }
  if (reading == b.stablePressed || nowMs - b.changedMs < DEBOUNCE_MS) return false;
  b.stablePressed = reading;
  if (reading) {
    b.pressedMs = nowMs;
    return true;
  }
  const bool isLong = nowMs - b.pressedMs >= LONG_PRESS_MS;
  if (isLong) {
    ++b.longPresses;
  } else {
    ++b.presses;
  }
  Serial.printf("[F6] %s(%d) %s, %lu ms\n", b.name, b.pin, isLong ? "길게" : "눌림", nowMs - b.pressedMs);
  drawLastEvent(b, isLong);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F6 button test");
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  for (Button &b : buttons) {
    pinMode(b.pin, INPUT_PULLUP);
    b.lastRead = b.stablePressed = digitalRead(b.pin) == LOW;
    Serial.printf("[F6] %s(%d) 시작 상태: %s\n", b.name, b.pin, b.stablePressed ? "눌림(배선 확인!)" : "뗌");
  }
  tft.init();
  tft.setRotation(ROTATION_PORTRAIT);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(0);
  u8f.setBackgroundColor(TFT_BLACK);
  drawHeader();
  drawCounts();
}

void loop() {
  const uint32_t nowMs = millis();
  bool changed = false;
  for (Button &b : buttons) changed |= updateButton(b, nowMs);
  if (changed) drawCounts();
  delay(2);
}
