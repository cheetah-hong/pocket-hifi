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
