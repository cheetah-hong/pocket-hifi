#include <Arduino.h>
// 35 ← 오른쪽 + 레일(아래쪽 반), 32 ← 오른쪽 − 레일(아래쪽 반)
void setup() {
  Serial.begin(115200);
  pinMode(32, INPUT_PULLUP);
  analogSetPinAttenuation(35, ADC_11db);
}
void loop() {
  int mv = analogReadMilliVolts(35);
  bool gnd = digitalRead(32) == LOW;
  Serial.printf("+레일 %d mV %s | -레일 %s\n", mv,
                mv > 2900 ? "OK" : "확인 필요", gnd ? "GND OK" : "끊김");
  delay(1000);
}
