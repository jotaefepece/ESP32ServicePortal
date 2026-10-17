#ifndef PORTAL_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif
#define PORTAL_MONITOR_H
#include <stddef.h>
void portal_print(const char *text);
void portal_println(const char *text);
void portal_monitor_read(char *out, size_t capacity);
void portal_monitor_clear(void);
void portal_led_enable(int gpio);
void portal_led_disable(void);
void portal_led_update(void);
#ifdef __cplusplus
}
#endif
#endif
