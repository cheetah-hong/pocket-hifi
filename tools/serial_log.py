# 사용: ~/.platformio/penv/bin/python -u tools/serial_log.py [포트] [초] > logs/이름.log
# 시리얼을 지정 시간 동안 계속 기록한다. serial_check.py와 달리 EN 리셋을 하지 않지만,
# 이 Mac·CP2102 조합은 포트를 여는 순간 한 번 리셋될 수 있다 → 블루투스 연결 전에 먼저 열어 둘 것.
import sys, time, serial
port = sys.argv[1] if len(sys.argv) > 1 else '/dev/cu.usbserial-0001'
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 3000
s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 0.2
s.dtr = False
s.rts = False
s.open()
end = time.time() + secs
while time.time() < end:
    line = s.readline()
    if line:
        print(line.decode('utf-8', 'replace').rstrip(), flush=True)
s.close()
