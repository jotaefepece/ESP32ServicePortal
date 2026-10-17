#include <ESP32ServicePortal.h>
ESP32ServicePortal portal;
unsigned long previous = 0;
void setup() {
  Serial.begin(115200);
  portal.begin();
  // Para un LED GPIO convencional, opcional:
  // portal.enableLED(2);
}
void loop() {
  if (millis() - previous >= 5000) {
    previous = millis();
    portal.println("Sistema funcionando");
    // Si activaste un LED GPIO convencional:
    // portal.refreshLED();
  }
}
