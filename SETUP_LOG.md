# ESP32 S1 준비 기록

- 2026-10-09: macOS 26.3.1 (25D2128), arm64. Git 2.50.1, PlatformIO Core 6.2.0 확인.
- CP2102 USB to UART Bridge Controller, Silicon Labs VID 0x10c4 / PID 0xea60, 포트 /dev/cu.usbserial-0001 확인. SPUSBDataType 출력이 없어 IORegistry로 확인.
- ~/.local/bin에 pio/platformio 링크 생성, ~/.zprofile에 PATH 추가.
- 요청한 네 파일을 원문 그대로 생성. 외부 핀 배선, sudo, 드라이버 설치, 보안 설정 변경 없음.
- 새 로그인 셸에서 pio 6.2.0 호출 성공. pyserial 3.5 사용 가능. git init 완료.
- system_profiler SPUSBHostDataType에서도 CP2102 VID 0x10c4 / PID 0xea60 확인.
- 첫 빌드 실행: espressif32 7.1.3 자동 설치, 툴체인 및 라이브러리 다운로드 진행. 버전 지정 변경 없음.
- 초기 sin1.contabostorage.com 미러 DNS 실패. PlatformIO 공식 미러 응답과 SHA-256을 확인한 대체 주소를 다운로드 캐시에 적용하고 설치 단계만 재시도. DNS/보안 설정, 프로젝트 코드, 버전 지정 변경 없음.
- 빌드 성공: RAM 21,464 / 327,680 bytes, Flash 269,045 / 3,145,728 bytes. 플랫폼 espressif32 7.1.3, Arduino 프레임워크 4.20017.260907+sha.dcc1105b.
- 보드 1 업로드 성공 및 쓰기 해시 검증. BOOT 버튼 조작 필요 없음. ESP32-D0WD-V3 revision v3.1, MAC 48:9d:31:04:5d:fc.
- 제공한 스크립트로 115200 baud, 8초 로그 확인: PocketHiFi boot / chip rev 3, cores 2, flash 4 MB / free heap 351548 bytes / alive 3회. 리셋 초기에 깨진 데이터가 있으나 이후 앱 로그 정상. 원본 로그 및 정상 부팅 구간은 logs/에 보존.
- 사용자에게 여분 보드 교체를 요청함. 완료 답변 전 보드 2 업로드 실행하지 않음.
- 보드 1 통과 후 F1 boot log 커밋 f0978f7 생성, f1 태그 부여.
- 사용자가 “여분 보드로 교체 완료”라고 답변. 포트 및 USB 칩 재확인: /dev/cu.usbserial-0001, CP2102 VID 0x10c4 / PID 0xea60.
- 보드 2 업로드 성공 및 쓰기 해시 검증. BOOT 버튼 조작 필요 없음. MAC 48:9d:31:04:5a:4c (보드 1과 다름).
- 보드 2 8초 로그 검증 통과: chip rev 3 / cores 2 / flash 4 MB / free heap 351548 / alive 3회. S1 두 보드 검증 완료.
- 원본 로그와 정상 앱 부팅 구간을 각각 보존. Claude Code 인수인계 파일에 최종 상태 반영.
- 2026-10-09: 최초 이동 시 경로를 잘못 해석하여 Projects 아래에 배치했고, 사용자 정정에 따라 기존 ~/project_hong/pocket-hifi로 이동을 바로잡음. Git 기록·f1 태그·소스·로그 유지, 문서 경로 반영.
- 이동 후 새 프로젝트 경로에서 pio run 성공 (7.03초).
- 최종 경로 ~/project_hong/pocket-hifi에서 pio run 성공 (6.96초).

# S2 브레드보드 전원 줄

- 2026-10-09: 사용자 배선 완료 — 3V3 → 왼쪽 바깥 + 레일, GND(왼쪽 14번 핀) → 왼쪽 바깥 − 레일, 47µF·0.1µF, 양쪽 판 바깥 레일 긴 점퍼 연결, 레일 가운데 끊김 짧은 점퍼 연결. 5V(VIN) 핀 미연결.
- S2: 멀티미터 대신 ESP32 ADC로 확인.
- 합선 확인: F1 그대로 8초 로그 — PocketHiFi boot / chip rev 3, cores 2, flash 4 MB / free heap 351548 / alive 3회. 3.3V–GND 합선 없음 (logs/s2-short-check.log).
- 확인용 점퍼: 왼쪽 판 6번 줄(35번 핀) G칸 → 오른쪽 판 바깥 + 레일 아래쪽 반, 7번 줄(32번 핀) G칸 → 오른쪽 판 바깥 − 레일 아래쪽 반. pocket-hifi.html S2 그림에 표시해 안내.
- [env:railcheck] (extends env:pocket, railcheck/railcheck.cpp) 업로드 후 10초 로그: "+레일 3134 mV OK | -레일 GND OK" 10회 연속 (logs/s2-railcheck-lower.log). 측정값 3134 mV는 ADC 11dB 상한 근처로 기준 2900 mV 이상. 32번 핀은 INPUT_PULLUP만 사용.
- 오른쪽 판 아래쪽 반에서 통과했으므로 왼쪽 레일 → 긴 점퍼 → 오른쪽 레일 → 가운데 연결까지 3.3V·GND 모두 이어짐. 위쪽 반은 별도 측정하지 않음.
- 확인용 점퍼 2개 제거 후 F1 재업로드, 8초 로그 정상: PocketHiFi boot / free heap 351548 / alive 3회 (logs/s2-f1-after.log).

# S3 화면 (F2)

- 2026-10-09: 단계 스케치를 src/stages/f1·f2·railcheck로 분리, platformio.ini에 [env:f1]·[env:f2]·[env:railcheck] 추가(모두 env:pocket extends). TFT 설정·라이브러리 버전 변경 없음.
- F2 첫 빌드 실패: TFT_eSPI User_Setup_Select.h의 `#define PIN_MOSI 8`이 pins.h의 PIN_MOSI와 충돌. pins.h의 SPI 상수 이름만 PIN_SPI_SCK/MOSI/MISO로 변경(핀 번호 18/23/19 그대로) 후 빌드 성공.
- 사용자 배선: 화면 VCC·LED → + 레일, GND → − 레일, CS → 27, RESET → 14, DC → 13, SDI → 23, SCK → 18. SDO·터치·SD_ 핀 미연결. 0.1µF는 선택 부품으로 HTML 그림에서 숨김.
- 보드 2(MAC 48:9d:31:04:5a:4c)에 F2 업로드, 시리얼 20초: tft 320x240, 색 채우기 → 영문·숫자 → 한글 → 세로 순서 반복 확인 (logs/f2-display.log).
- 사용자 눈으로 확인: 빨강·초록·파랑 정상, 영문·숫자 정상, 한글 "안녕, 포켓 하이파이" 정상, 세로 방향 정상. 어두움·줄 깨짐 없음. S3 통과, 태그 f2.

# S4 microSD (F3)

- 2026-10-09: SD 보드(파란색, 핀 3v3·CS·MOSI·CLK·MISO·GND, 레귤레이터 없음) 확인. HTML S4 그림을 실물 핀 순서로 수정.
- 카드: 1GB FAT32 "NO NAME". /music 생성, 사용자가 넣은 24곡을 ID3 태그 기준 /music/The Weeknd/Dawn FM (Alternate World)/로 정리, 내장 앨범 아트로 cover.jpg(240×240 baseline) 생성, macOS ._ 파일 정리 후 추출.
- 사용자 배선: 3v3 → + 레일, GND → − 레일, CS → 21, MOSI → 23, CLK → 18, MISO → 19. 화면 배선 유지, 화면 SDO 미연결.
- [env:f3] 추가. 초기화 순서 화면 → SD, SD.begin에 tft.getSPIinstance()(TFT_eSPI 자체 VSPI 객체)를 넘겨 같은 SPI 인스턴스 사용. 숨김(.) 항목은 건너뜀.
- 시리얼 확인(직접): SD mount OK @ 20 MHz, type 2(SDHC), 982 MB, 곡 24개 "경로 (KB)" 출력, …·’ 문자 정상 (logs/f3-sd-list.log). mount 중 sd_diskio "token error [8] 0x5" 경고 1회 있으나 mount·목록 정상.
- 화면 확인(사용자): "곡 24개 · /music"과 곡 목록 표시, 깨진 글자 없음. 카드에 한글 파일명이 없어 한글 파일명 표시는 미검증. S4 통과, 태그 f3.
- F3 추가 · 앨범 커버: [env:f3cover]에만 bodmer/TJpg_Decoder 추가(1.1.0 설치, 버전 미고정, 사용자 동의). /music/<아티스트>/<앨범>/cover.jpg를 세로 화면 위 240×240에 표시, 아래에 앨범·아티스트·곡 수.
- 첫 업로드에서 리셋 직후 SD mount 실패(GO_IDLE_STATE failed). 원인: 화면 초기화 중 SD CS(21)가 떠 있어 카드가 버스 신호를 받음. tft.init 전에 SD CS를 OUTPUT HIGH로 고정하자 리셋 3회 모두 첫 시도 20 MHz mount 성공. 재시도(20 MHz 2회 → 4 MHz 2회)도 남겨 둠.
- 커버 디코드 약 162 ms (logs/f3cover.log). 사용자 확인: 커버·앨범명·아티스트·곡 수 정상 표시.

# S6 블루투스 1차 (F5, DAC 전)

- 2026-10-09: 멀티미터 배송 전이라 S5보다 S6 연결·화면 연동을 먼저 시험. [env:f5] 추가, I2S 출력 끄고(set_stream_reader(..., false)) 수신량만 측정, AVRCP 곡 정보·재생 상태를 세로 화면에 표시. BT 이름 PocketHiFi.
- 빌드 실패: audio-tools esp32.h의 `#define PIN_I2S_BCK 14`가 pins.h와 충돌. pins.h I2S 상수 이름만 PIN_DAC_BCK/LRCK/DATA로 변경(핀 26/25/22 그대로). 라이브러리 매크로와 겹치는 pins.h 이름 전수 확인, 남은 충돌 없음. f1·f2·f3·f3cover·railcheck 재빌드 성공.
- Mac 연결: 연결·재생/일시정지 정상. 소리 출력을 PocketHiFi로 직접 바꾼 뒤 수신 172~176 KB/s(44.1 kHz 스테레오 16비트와 일치). 곡 정보는 ESP32가 metadata 요청을 보냈으나 Mac 응답 없음 → 제목·아티스트·앨범 미표시.
- 폰 연결: 제목·아티스트·앨범 모두 수신·표시(사용자 확인). 한글 제목 "영원은 아니어도" 정상. 수신 172 KB/s (logs/f5-bt-mac.log).
- 로그에 i2s_set_clk failed(44100) 1회: I2S 출력을 꺼 둔 상태라 발생, DAC 연결(S5) 후 재확인 필요.
- S6 기준 중 "30분 연속 재생"은 DAC 이후로 보류. 태그 f5는 S6 완료 시 붙임.
