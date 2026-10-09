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
프로젝트 위치: ~/Projects/project_hong/pocket-hifi

```sh
cd ~/Projects/project_hong/pocket-hifi
pio run
pio run -t upload --upload-port /dev/cu.usbserial-0001
~/.platformio/penv/bin/python tools/serial_check.py /dev/cu.usbserial-0001 8
```

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
