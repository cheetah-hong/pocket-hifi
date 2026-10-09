# PocketHiFi 개발 인수인계

## 하드웨어 및 현재 상태
- 사용자 제공 모델: ESP32-WROOM-32D DevKitC, USB-C, 38핀.
- 보드 1 USB 칩: Silicon Labs CP2102 USB to UART Bridge Controller, VID 0x10c4 / PID 0xea60.
- 보드 1 포트: /dev/cu.usbserial-0001. 보드를 교체하면 포트를 다시 확인한다.
- macOS 26.3.1, Apple Silicon (arm64), PlatformIO Core 6.2.0, Git 2.50.1.
- 보드 단독 테스트. 외부 부품이나 핀 배선 없음.
- 두 보드 모두 S1 빌드·업로드·부팅 검증 통과. BOOT 버튼 조작 필요 없음.
- 현재 연결된 보드 2도 CP2102 (VID 0x10c4 / PID 0xea60), 포트 /dev/cu.usbserial-0001.
- 보드 1 MAC 48:9d:31:04:5d:fc, 보드 2 MAC 48:9d:31:04:5a:4c.
- 업로드로 확인한 칩: ESP32-D0WD-V3 revision v3.1, 2코어, 플래시 4 MB. 앱 여유 힙 351,548 bytes.
- 확인한 칩 정보만으로 모듈의 WROOM-32D 표기를 확정하지는 않았다.
- logs/board1-serial-raw.log 및 logs/board2-serial-raw.log에 원본 로그를, logs/board1-boot.log 및 logs/board2-boot.log에 정상 앱 부팅 구간을 보존했다.
- 두 보드 모두 리셋 직후 깨진 문자가 있었으나 115200 baud에서 앱 부팅 이후 출력은 정상이다.
- 첫 설치 중 sin1.contabostorage.com DNS 실패가 발생했으나 공식 대체 미러와 SHA-256 검증으로 설치를 완료했다. 이미 설치된 도구로 터미널 빌드·업로드·로그 확인이 가능하다.
- f1 태그는 보드 1 최초 통과 커밋에 있다. 여분 보드 검증과 최종 인수인계는 후속 커밋에 있다. 다음 개발 단계는 f2부터 진행한다.

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
- 현재 단계: f1(부팅 로그), f2(화면·한글), f3(microSD 곡 목록), f3cover(앨범 커버, TJpg_Decoder 사용), f5(블루투스 A2DP 수신·AVRCP 곡 정보, 소리 출력 없음), railcheck(S2 레일 확인). [env:pocket] 단독은 f1과 같다.
- 업로드: `pio run -e f2 -t upload --upload-port /dev/cu.usbserial-0001`
- 새 단계 추가: src/stages/f3/main.cpp 생성 → platformio.ini에 [env:f3] (extends = env:pocket, build_src_filter = -<*> +<pins.h> +<stages/f3/>).
- pins.h의 SPI 상수는 PIN_SPI_SCK/MOSI/MISO, I2S 상수는 PIN_DAC_BCK/LRCK/DATA다. TFT_eSPI(PIN_MOSI·PIN_MISO·PIN_SCLK)와 audio-tools(PIN_I2S_*)가 같은 이름의 매크로를 정의하므로 그 이름은 쓰지 않는다.
- TFT_eSPI 핀은 platformio.ini build_flags에서 오므로, 화면을 쓰는 스케치는 static_assert로 pins.h와 같은지 확인한다.

### GitHub
- 원격: https://github.com/cheetah-hong/pocket-hifi (public). 로컬 S2 브랜치 → 원격 main.
- 단계를 통과할 때마다 커밋·태그 후 푸시한다. 커밋 메시지는 한글, Claude 관련 문구나 공동 작성자 표시를 넣지 않는다.
- 브랜치 S2와 태그 s2는 macOS에서 이름이 겹치므로 refs/heads/S2, refs/tags/s2처럼 전체 이름을 쓴다.
  예: `git push origin refs/heads/S2:refs/heads/main refs/tags/f2`

- pio 및 platformio는 ~/.local/bin에 연결되어 있고 ~/.zprofile에서 PATH에 추가했다.
- 포트 재확인: `ls /dev/cu.*`, `pio device list`.
- 이 맥에서 USB 칩 확인: `system_profiler SPUSBHostDataType` (SPUSBDataType은 출력 없음).
- 로그 스크립트는 115200 baud로 EN 리셋 후 지정한 시간 동안 출력한다.
- 통과 기준: PocketHiFi boot, chip rev/cores/flash(4 MB 이상), free heap, 약 2초 간격 alive 출력.

## 개발 규칙
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
- 블루투스 연결 중 로그는 tools/serial_check.py로 읽지 않는다(EN 리셋으로 연결이 끊김). 이 Mac·CP2102 조합은 포트를 여는 순간에도 리셋되므로, 연결 전에 시리얼을 먼저 열어 두고 계속 기록한다.
