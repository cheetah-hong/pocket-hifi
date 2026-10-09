#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("PocketHiFi boot");
  Serial.printf("chip rev %d, cores %d, flash %u MB\n",
    ESP.getChipRevision(), ESP.getChipCores(), ESP.getFlashChipSize() / 1048576);
  Serial.printf("free heap %u bytes\n", ESP.getFreeHeap());
}

void loop() {
  delay(2000);
  Serial.printf("alive, heap %u\n", ESP.getFreeHeap());
}
