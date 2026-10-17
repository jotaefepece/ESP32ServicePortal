#pragma once
#include <Arduino.h>
class ESP32ServicePortal {
public:
    bool begin();
    void print(const char* text);
    void println(const char* text);
    void print(const String& text);
    void println(const String& text);
    void println();
    bool connected() const;
    String ip() const;
    String ssid() const;
    void enableLED(int gpio);
    void disableLED();
    void refreshLED();
    void clearMonitor();
};
