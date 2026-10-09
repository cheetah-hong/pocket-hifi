// src/pins.h — 핀은 여기에만
#pragma once
constexpr int PIN_SCK = 18, PIN_MOSI = 23, PIN_MISO = 19;      // 공통 SPI
constexpr int PIN_TFT_CS = 27, PIN_TFT_DC = 13, PIN_TFT_RST = 14;
constexpr int PIN_SD_CS = 21;
constexpr int PIN_I2S_BCK = 26, PIN_I2S_LRCK = 25, PIN_I2S_DATA = 22;
constexpr int PIN_ENC_A = 32, PIN_ENC_B = 33, PIN_ENC_SW = 34; // V0: 32·33은 위/아래 버튼
constexpr int PIN_BTN_MENU = 16, PIN_BTN_PLAY = 17;
constexpr int PIN_BAT_ADC = 35;
constexpr int PIN_DAC_MUTE = 4;
