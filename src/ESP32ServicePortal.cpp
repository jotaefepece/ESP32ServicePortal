#include "ESP32ServicePortal.h"
#include "sistema.h"
#include "wifi_manager.h"
#include "dashboard.h"
#include "portal_monitor.h"
static bool portal_started = false;
bool ESP32ServicePortal::begin() {
    if (portal_started) return true;
    inicializar_sistema();
    inicializar_nvs();
    inicializar_wifi();
    iniciar_wifi_ap();
    iniciar_dashboard();
    portal_started = true;
    portal_println("ESP32 Service Portal iniciado");
    return true;
}
void ESP32ServicePortal::print(const char* s) { portal_print(s); }
void ESP32ServicePortal::println(const char* s) { portal_println(s); }
void ESP32ServicePortal::print(const String& s) { portal_print(s.c_str()); }
void ESP32ServicePortal::println(const String& s) { portal_println(s.c_str()); }
void ESP32ServicePortal::println() { portal_println(""); }
bool ESP32ServicePortal::connected() const { return wifi_esta_conectado(); }
String ESP32ServicePortal::ip() const { return String(wifi_obtener_ip()); }
String ESP32ServicePortal::ssid() const { return String(wifi_obtener_ssid()); }
void ESP32ServicePortal::enableLED(int gpio) { portal_led_enable(gpio); }
void ESP32ServicePortal::disableLED() { portal_led_disable(); }
void ESP32ServicePortal::refreshLED() { portal_led_update(); }
void ESP32ServicePortal::clearMonitor() { portal_monitor_clear(); }
