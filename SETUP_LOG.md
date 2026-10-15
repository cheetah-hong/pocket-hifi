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

# S5 준비 · 멀티미터 실측 (S2 보완)

- 2026-10-11: 멀티미터 도착. 처음 측정에서 약 50 mV — 리드가 레일 금속에 닿지 않은 측정 문제(화면은 켜져 있었음). 점퍼선 금속 끝에 대고 다시 측정.
- 레일 네 곳(왼쪽 위·아래, 오른쪽 위·아래) 3.23~3.27 V (F5 실행 중, 화면·SD 연결 상태). ESP32 동작 범위(3.0~3.6 V) 안이고 기준 3.25~3.35 V 하한에 근접. 위·아래 반쪽, 양쪽 판 모두 전원 연결 확인 → S2 실측 보완.

# S5 소리 1차 · DAC (F4)

- 2026-10-11: DAC(PCM5102A 보라색 보드) 설정 패드는 배송 때부터 H1L·H2L·H4L = L, H3L(XSMT) = H로 납땜돼 있었음. 도통으로 오른쪽 패드 = GND(L) 방향 확인, 가운데 패드: H1L·H2L·H4L 삑, H3L 조용 → W4 작업 없이 통과.
- 사용자 납땜: DAC 아래쪽 6핀 헤더(핀이 뒷면 방향). 위쪽 LROUT·AGND·ROUT 헤더는 미납땜(앰프 단계에서).
- 배선(DAC 판 밖, 수-암 점퍼): VIN → + 레일, GND·SCK → − 레일, LCK → 25, BCK → 26, DIN → 22. HTML S5에 그림 추가.
- [env:f4] 추가: SD /music mp3 재귀 수집 → Helix MP3 디코드 → VolumeStream → I2S(BCK 26, WS 25, DATA 22). 화면에 재생 곡 표시. 업로드 직후 첫 시리얼 읽기는 깨진 문자만 나왔으나 재리셋 후 정상.
- 재생 속도: 5초마다 약 79 KB 진행(약 16 KB/s, 128 kbps 실시간과 일치), heap 248 KB 일정 (logs/f4-dac.log).
- 사용자 확인(DAC 잭 + 이어폰): 볼륨 0.1 → 0.3 → 0.7(사용자 요청), 2번 트랙부터. 양쪽 모두 들리고 잡음 없음. 저음이 강하고 중역이 빈 느낌 — 라인 출력을 저임피던스 이어폰에 직접 물린 영향으로 추정, 앰프(TDA1308) 연결 후 재확인.
- 앰프(TDA1308) 시도: 헤더가 없어 점퍼선을 잘라 직접 납땜(전원 +·− → 레일, IN ← DAC LROUT·AGND·ROUT, OUT → PJ-392A). 잭은 Ω 측정으로 가운데 긴 다리 = GND 확인(가운데↔양옆 35·34 Ω, 양옆끼리 65 Ω). 도통 확인은 모두 정상.
- 앰프 경로 소리: 볼륨 0.05에서도 크고 깨지며 떨림. DAC 잭 직결은 같은 조건에서 깨끗 → 앰프 쪽 문제. 점퍼선 가닥이 쉽게 끊어져 납땜 품질이 불확실하다는 사용자 판단에 따라 앰프는 V1(만능기판·AWG26 연선)로 연기. HTML 가이드 "앰프는 미뤄도 됩니다"에 해당.
- S5 통과(DAC 잭 기준): SD mp3 실시간 재생, 양쪽 소리, 잡음 없음. F4 볼륨 0.7(DAC 잭 직결). 진단용 [env:f4lr](좌 440 Hz / 우 880 Hz 번갈아, 0.02) 추가. 태그 f4.

# S6 블루투스 2차 · DAC 출력 (F5)

- 2026-10-15: 첫 DAC가 앰프 입력선이 남은 채 왼쪽 소리가 안 나와 예비 DAC로 교체. SD 재생(F4)은 양쪽 정상.
- F5에 I2S DAC 출력 추가(BCK 26, WS 25, DATA 22, DMA 버퍼 16×512). 폰 연결 시 0.01~0.1초 끊김이 수시로 발생.
  - 원인 1: CORE_DEBUG_LEVEL=3의 BT 라이브러리 INFO 로그가 BT 작업을 붙잡음 → [env:f5]에서만 build_unflags로 1로 낮춤(공통·TFT 설정 변경 없음).
  - 원인 2: BluetoothA2DPSinkQueued로 바꾸자 여유 heap 약 48 KB, 수신 144~152 KB/s로 부족(필요 약 172 KB/s). 폰을 붙이고 Wi-Fi를 꺼도 같아서 무선 문제 아님.
  - 기본 BluetoothA2DPSink로 되돌림(디버그 1 + DMA 16 유지) → 수신 170 KB/s, 끊김 없음(사용자 확인).
- 화면: 하단 수신 KB/s·수신 끊김(80 ms 이상 데이터 공백)·재생 시간 표시, 5초마다 갱신. 세로 화면 규칙을 CLAUDE.md에 추가, F4·F4LR도 세로로 변경.
- (이어서) 기본 수신기로도 1분 뒤 136 KB/s로 떨어지고 끊김 재발 → 진단으로 원인 분리(모두 같은 폰·같은 보드):
  - 진단 1 I2S 출력 끔: 172~176 KB/s 안정. → 출력 쪽이 원인.
  - 직접 만든 비차단 구조(BT 콜백 → 16 KB 스트림 버퍼 → 출력 작업): 그래도 128~150. → BT가 I2S를 기다리는 것이 원인 아님.
  - 진단 2 DAC 신호선 3개 분리: 그대로 128~150. → DAC·배선 잡음 아님.
  - 진단 3 I2S 없이 메모리 40 KB 점유: 172. → 메모리 부족 아님.
  - 진단 4 I2S 켜고 무음, 출력 작업 없음: 172, 볼륨 정상. → I2S 장치 자체 아님.
  - 진단 5 출력 작업 정상, DAC로 보낼 데이터만 0: 172~176, 볼륨 정상. → GPIO 22(DIN)의 데이터 깜빡임이 원인. 22는 모듈 안테나 바로 아래.
- 사용자 동의로 DAC DATA를 GPIO 22 → 4(오른쪽 판 13번 줄)로 이동, pins.h의 PIN_DAC_MUTE(4) 삭제(XSMT 패드 고정이라 미사용). HTML 배선 그림·핀 표 수정. 스트림 버퍼 32 KB, 재생 시작 전 20 KB 모음.
- S6 통과: 폰 연결 30분 연속 재생(logs/f5-din4-30min.log). 수신 172~176 KB/s 30분 내내 유지, 넘침 0, 재연결 0, 재부팅 0, heap 43~44 KB 일정. 사용자 확인: 귀로 들리는 끊김 없음, 볼륨 버튼 반영 정상, 이전 DAC에서 느낀 "가운데 빈 느낌" 없음.
- 측정 정리: 기존 "빔"(중간 버퍼가 잠깐 빈 횟수, 30분에 408)은 DMA 버퍼(약 186 ms)가 메워 소리와 무관 → "끊김"(쓰기 공백이 DMA 보유 시간 초과)으로 교체. 화면 하단 1초 갱신(사용자 요청)도 영향 없음 확인. 태그 f5.
- 음색 비교: 새 DAC로 SD "Out of Time"(F4, 볼륨 0.35, 한 곡 반복)을 들은 결과 사용자 "음향 합격". 첫 DAC에서 느낀 저음 과다·가운데 빈 느낌은 첫 DAC 보드 문제로 결론(그 보드는 이후 왼쪽 출력이 사라짐). F4에 START_TRACK·REPEAT_ONE 상수 추가.

# S7 준비 (F6)

- 2026-10-15: HTML에 S7 버튼 배선 그림·순서 추가(버튼은 34~36·38~40번 줄 — 31~33번 줄은 레일이 끊긴 구간). [env:f6] 버튼 테스트(32·33·16·17 내부 풀업, 30 ms 디바운스, 0.6초 길게 누름, 세로 화면) 빌드·업로드, 버튼 미배선 상태에서 네 버튼 모두 "뗌"으로 부팅 확인. 사용자 배선 대기.
- tools/serial_log.py 추가(장시간 시리얼 기록용, S6에서 쓰던 임시 스크립트를 저장소로 옮김). CLAUDE.md에 다음 작업(S7·F7) 인수인계 정리.
- 2026-10-16: 사용자 배선 완료(가이드 ①~⑤) 및 F6 버튼 테스트 직접 확인 — 위·아래·MENU·PLAY 모두 한 번 누르면 한 번만 셈, 0.6초 이상 길게 누름 인식. 시리얼 리셋 후 네 버튼 모두 시작 상태 "뗌"(신호선이 GND 쪽에 꽂힌 버튼 없음, logs/f6-buttons-boot.log). F6 통과, 태그 f6.
