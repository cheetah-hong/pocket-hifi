// src/pins.h — 핀은 여기에만
#pragma once
constexpr int PIN_SPI_SCK = 18, PIN_SPI_MOSI = 23, PIN_SPI_MISO = 19;  // 공통 SPI (TFT_eSPI의 PIN_MOSI 매크로와 겹치지 않게)
constexpr int PIN_TFT_CS = 27, PIN_TFT_DC = 13, PIN_TFT_RST = 14;
constexpr int PIN_SD_CS = 21;
// I2S DAC (audio-tools의 PIN_I2S_* 매크로와 겹치지 않게).
// DATA는 22 → 4: 22는 안테나 바로 아래라 음악 데이터 깜빡임이 블루투스 수신을 떨어뜨림(S6 진단).
constexpr int PIN_DAC_BCK = 26, PIN_DAC_LRCK = 25, PIN_DAC_DATA = 4;
constexpr int PIN_ENC_A = 32, PIN_ENC_B = 33, PIN_ENC_SW = 34; // V0: 32·33은 위/아래 버튼
constexpr int PIN_BTN_MENU = 16, PIN_BTN_PLAY = 17;
constexpr int PIN_BAT_ADC = 35;
// PIN_DAC_MUTE(4) 삭제: 이 DAC는 XSMT 패드가 H로 납땜돼 GPIO 제어 불가, 4번은 DAC DATA로 사용.
