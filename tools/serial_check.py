# 사용: ~/.platformio/penv/bin/python tools/serial_check.py [포트] [초]
import sys, time, glob, serial
cands = glob.glob('/dev/cu.usbserial-*') + glob.glob('/dev/cu.SLAB_USBtoUART') + glob.glob('/dev/cu.wchusbserial*')
port = sys.argv[1] if len(sys.argv) > 1 else (cands[0] if cands else sys.exit('포트 없음'))
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 8
s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 0.2
s.dtr = False; s.rts = False
s.open()
s.rts = True; time.sleep(0.1); s.rts = False      # EN 리셋 (IO0은 건드리지 않음)
end = time.time() + secs
while time.time() < end:
    line = s.readline()
    if line:
        print(line.decode(errors='replace').rstrip())
s.close()
