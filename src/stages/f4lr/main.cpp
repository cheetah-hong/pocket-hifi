// F4 보조 · 좌우 확인: 왼쪽 채널만 440 Hz 2초 → 오른쪽 채널만 880 Hz 2초를 반복
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include "AudioTools.h"
#include "pins.h"

static_assert(TFT_CS == PIN_TFT_CS && TFT_DC == PIN_TFT_DC && TFT_RST == PIN_TFT_RST,
              "platformio.ini TFT 핀이 pins.h와 다름");

constexpr uint32_t SAMPLE_RATE = 44100;
constexpr float TONE_VOLUME = 0.02f;  // 앰프 경로 진단용으로 아주 작게
constexpr float LEFT_HZ = 440.0f;
constexpr float RIGHT_HZ = 880.0f;
constexpr uint32_t SIDE_MS = 2000;
constexpr size_t FRAMES_PER_CHUNK = 256;

TFT_eSPI tft;
U8g2_for_TFT_eSPI u8f;
I2SStream i2s;

void showSide(const char *side, const char *pitch, uint16_t color) {
  tft.fillScreen(TFT_BLACK);
  u8f.setForegroundColor(color);
  u8f.setCursor(20, 100);
  u8f.print(side);
  u8f.setForegroundColor(TFT_WHITE);
  u8f.setCursor(20, 130);
  u8f.print(pitch);
}

// left=true면 왼쪽 채널에만, 아니면 오른쪽 채널에만 사인파를 쓴다.
void playTone(bool left, float hz, uint32_t ms) {
  static int16_t frames[FRAMES_PER_CHUNK * 2];
  static float phase = 0;
  const float step = 2.0f * PI * hz / SAMPLE_RATE;
  const uint32_t end = millis() + ms;
  while (millis() < end) {
    for (size_t i = 0; i < FRAMES_PER_CHUNK; ++i) {
      const int16_t v = static_cast<int16_t>(sinf(phase) * 32767 * TONE_VOLUME);
      phase += step;
      if (phase > 2 * PI) phase -= 2 * PI;
      frames[i * 2] = left ? v : 0;
      frames[i * 2 + 1] = left ? 0 : v;
    }
    i2s.write(reinterpret_cast<uint8_t *>(frames), sizeof(frames));
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi F4LR left/right test");
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  tft.init();
  tft.setRotation(1);
  u8f.begin(tft);
  u8f.setFont(u8g2_font_unifont_t_korean2);
  u8f.setFontMode(1);

  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck = PIN_DAC_BCK;
  cfg.pin_ws = PIN_DAC_LRCK;
  cfg.pin_data = PIN_DAC_DATA;
  cfg.sample_rate = SAMPLE_RATE;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  i2s.begin(cfg);
}

void loop() {
  Serial.println("[F4LR] 왼쪽 440 Hz");
  showSide("왼쪽(L)", "낮은 삐 440 Hz", TFT_CYAN);
  playTone(true, LEFT_HZ, SIDE_MS);
  Serial.println("[F4LR] 오른쪽 880 Hz");
  showSide("오른쪽(R)", "높은 삐 880 Hz", TFT_ORANGE);
  playTone(false, RIGHT_HZ, SIDE_MS);
}
