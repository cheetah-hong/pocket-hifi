# PocketHiFi 개발 인수인계

## 하드웨어 및 현재 상태
- 사용자 제공 모델: ESP32-WROOM-32D DevKitC, USB-C, 38핀.
- 보드 1 USB 칩: Silicon Labs CP2102 USB to UART Bridge Controller, VID 0x10c4 / PID 0xea60.
- 보드 1 포트: /dev/cu.usbserial-0001. 보드를 교체하면 포트를 다시 확인한다.
- macOS 26.3.1, Apple Silicon (arm64), PlatformIO Core 6.2.0, Git 2.50.1.
- 브레드보드 2개에 ESP32(보드 2)를 걸쳐 꽂고 화면(ILI9341)·microSD·DAC(PCM5102A 예비 보드, 판 밖 점퍼 연결)을 배선한 상태. 앰프(TDA1308)는 V1로 연기.
- 두 보드 모두 S1 빌드·업로드·부팅 검증 통과. BOOT 버튼 조작 필요 없음.
- 현재 연결된 보드 2도 CP2102 (VID 0x10c4 / PID 0xea60), 포트 /dev/cu.usbserial-0001.
- 보드 1 MAC 48:9d:31:04:5d:fc, 보드 2 MAC 48:9d:31:04:5a:4c.
- 업로드로 확인한 칩: ESP32-D0WD-V3 revision v3.1, 2코어, 플래시 4 MB. 앱 여유 힙 351,548 bytes.
- 확인한 칩 정보만으로 모듈의 WROOM-32D 표기를 확정하지는 않았다.
- logs/board1-serial-raw.log 및 logs/board2-serial-raw.log에 원본 로그를, logs/board1-boot.log 및 logs/board2-boot.log에 정상 앱 부팅 구간을 보존했다.
- 두 보드 모두 리셋 직후 깨진 문자가 있었으나 115200 baud에서 앱 부팅 이후 출력은 정상이다.
- 첫 설치 중 sin1.contabostorage.com DNS 실패가 발생했으나 공식 대체 미러와 SHA-256 검증으로 설치를 완료했다. 이미 설치된 도구로 터미널 빌드·업로드·로그 확인이 가능하다.
- 통과 태그: f1(S1) · s2(S2) · f2(S3 화면) · f3(S4 SD) · f4(S5 DAC 재생) · f5(S6 블루투스 30분) · f6(S7 버튼 4개). 상세는 SETUP_LOG.md.

## 다음 작업: S7 버튼 4개 (F6 → F7)
- 사람용 배선 가이드: ~/Downloads/pocket-hifi.html의 "S7 배선 · 버튼 4개" 그림과 순서 ①~⑤ (저장소 밖 파일). 버튼은 두 판 아래쪽 34~36·38~40번 줄, 가운데 홈을 가로질러. 신호: 위 → 왼쪽 판 7번 줄(32), 아래 → 8번 줄(33), MENU → 오른쪽 판 12번 줄(16), PLAY → 11번 줄(17). GND는 가까운 − 레일. 34번 핀은 비워 둔다.
- 2026-10-16: 버튼 배선 완료, F6 통과(태그 f6). 다음은 F7.
- F6 통과 기준: 버튼마다 한 번 누르면 한 번만 셈(30 ms 디바운스), 0.6초 이상은 "길게", 시작 시 "눌림"으로 읽히는 버튼이 없을 것(있으면 신호선이 GND 쪽에 꽂힌 것). 통과하면 태그 f6.
- 그다음 F7 화면 흐름(세로): 부팅 화면(자체 로고, 남의 상표 금지) → 곡 목록(위·아래·PLAY 선택, MENU 뒤로) → 재생 화면(앨범 커버 cover.jpg + 제목·아티스트) → SD/블루투스 전환. 블루투스 재생 시 커버는 SD에 같은 앨범 폴더가 있으면 그 cover.jpg, 없으면 기본 그림(사용자와 합의한 방향). 화면 갱신은 1초 주기가 적당(사용자 요청, S6에서 소리 영향 없음 확인).
- 남은 V0 이후 메모: 앰프는 V1 만능기판에서 AWG26 연선으로 재시도. 사용자 이어폰 기준 DAC 잭 볼륨은 0.35~0.7이 적당했다.

## 터미널 명령
프로젝트 위치: ~/project_hong/pocket-hifi

```sh
cd ~/project_hong/pocket-hifi
pio run
pio run -t upload --upload-port /dev/cu.usbserial-0001
~/.platformio/penv/bin/python tools/serial_check.py /dev/cu.usbserial-0001 8
```

### 단계 스케치 (src/stages)
- 단계마다 스케치를 지우지 않고 src/stages/<이름>/main.cpp로 보존한다. 공통 핀은 src/pins.h.
- platformio.ini의 [env:<이름>]이 [env:pocket]을 extends하고 build_src_filter로 해당 폴더만 빌드한다.
- 현재 단계: f1(부팅 로그), f2(화면·한글), f3(microSD 곡 목록), f3cover(앨범 커버, TJpg_Decoder 사용), f4(SD mp3 → I2S DAC 재생, 볼륨·시작 곡·한 곡 반복 상수), f4lr(좌우 채널 확인 톤), f5(블루투스 → 버퍼 → DAC, AVRCP 곡 정보·수신량·끊김 표시), f6(버튼 입력 테스트), railcheck(S2 레일 확인). [env:pocket] 단독은 f1과 같다.
- 업로드: `pio run -e f2 -t upload --upload-port /dev/cu.usbserial-0001`
- 새 단계 추가: src/stages/f3/main.cpp 생성 → platformio.ini에 [env:f3] (extends = env:pocket, build_src_filter = -<*> +<pins.h> +<stages/f3/>).
- pins.h의 SPI 상수는 PIN_SPI_SCK/MOSI/MISO, I2S 상수는 PIN_DAC_BCK/LRCK/DATA다. TFT_eSPI(PIN_MOSI·PIN_MISO·PIN_SCLK)와 audio-tools(PIN_I2S_*)가 같은 이름의 매크로를 정의하므로 그 이름은 쓰지 않는다.
- TFT_eSPI 핀은 platformio.ini build_flags에서 오므로, 화면을 쓰는 스케치는 static_assert로 pins.h와 같은지 확인한다.

### GitHub
- 원격: https://github.com/cheetah-hong/pocket-hifi (public). 로컬 cheetah-hong/S7 브랜치(S7 워크트리) → 원격 main. S6까지는 S2 워크트리의 S2 브랜치에서 푸시했다.
- 단계를 통과할 때마다 커밋·태그 후 푸시한다. 커밋 메시지는 한글, Claude 관련 문구나 공동 작성자 표시를 넣지 않는다.
- 브랜치와 태그 이름이 겹칠 수 있으므로(예: 예전 브랜치 S2와 태그 s2는 macOS에서 겹침) refs/heads/…, refs/tags/…처럼 전체 이름을 쓴다.
  예: `git push origin refs/heads/cheetah-hong/S7:refs/heads/main refs/tags/f6`

- pio 및 platformio는 ~/.local/bin에 연결되어 있고 ~/.zprofile에서 PATH에 추가했다.
- 포트 재확인: `ls /dev/cu.*`, `pio device list`.
- 이 맥에서 USB 칩 확인: `system_profiler SPUSBHostDataType` (SPUSBDataType은 출력 없음).
- 로그 스크립트는 115200 baud로 EN 리셋 후 지정한 시간 동안 출력한다.
- 오래 기록할 때(블루투스 등)는 tools/serial_log.py를 백그라운드로 돌려 logs/에 남긴다. 업로드하려면 먼저 그 기록 프로세스를 끝내야 한다(포트 점유).
- 통과 기준: PocketHiFi boot, chip rev/cores/flash(4 MB 이상), free heap, 약 2초 간격 alive 출력.

## 개발 규칙
- 화면 UI는 모두 세로 기준(setRotation 0, 240×320)으로 작성한다. 사용자가 화면을 세로로 고정해 쓴다.
- 핀 번호는 src/pins.h에만 쓴다. 초기 platformio.ini의 TFT 설정은 사용자 제공 원문을 유지했다.
- 단계마다 커밋하고 태그(f1, f2 …)를 붙인다.
- 오디오 볼륨은 처음에 0.1로 시작한다.
- 배선 변경이나 BOOT 버튼처럼 몸으로 하는 일은 사람에게 요청하고, 완료 답변을 기다린다.
- 5V(VIN) 핀은 어떤 모듈에도 연결하지 않는다.
- sudo, 드라이버·커널 확장 설치, 보안 설정 변경은 먼저 사용자에게 확인한다.
- 라이브러리 버전 문제로 빌드가 실패하면 버전 고정이나 코드 수정 없이 오류 원문을 보고한다.
- 연결 실패 시 사람에게 요청: “보드의 BOOT 버튼을 누른 채로 있다가, Connecting...이 보이면 떼 주세요.”

## 검증 기록
SETUP_LOG.md를 확인한다. 실제 완료 여부는 그 기록과 Git 태그로 확인한다.
- DAC DATA는 GPIO 4(오른쪽 판 13번 줄). GPIO 22는 안테나 바로 아래라 I2S 데이터가 블루투스 수신을 떨어뜨린다(S6 진단). 안테나 근처 핀(22·23 등)에 빠른 신호를 새로 배정하지 않는다.
- F5 오디오 구조: BT 콜백은 32 KB 스트림 버퍼에 넣기만 하고, 별도 출력 작업(core 1)이 볼륨 적용 후 I2S로 쓴다. BT 콜백에서 I2S를 기다리면 수신이 떨어진다.
- 블루투스 연결 중 로그는 tools/serial_check.py로 읽지 않는다(EN 리셋으로 연결이 끊김). 이 Mac·CP2102 조합은 포트를 여는 순간에도 리셋되므로, 연결 전에 시리얼을 먼저 열어 두고 계속 기록한다.
