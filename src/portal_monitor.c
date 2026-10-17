#include "portal_monitor.h"
#include "wifi_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#define CAP 4096
static char buf[CAP];
static size_t used;
static SemaphoreHandle_t mutex;
static int led = -1;
static void init_mutex(void) { if (!mutex) mutex = xSemaphoreCreateMutex(); }
static void append(const char *s) {
    if (!s) return;
    init_mutex();
    size_t n = strlen(s);
    if (n >= CAP) { s += n - CAP + 1; n = CAP - 1; }
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) return;
    size_t drop = used + n >= CAP ? used + n - CAP + 1 : 0;
    if (drop) { memmove(buf, buf + drop, used - drop); used -= drop; }
    memcpy(buf + used, s, n);
    used += n;
    buf[used] = 0;
    xSemaphoreGive(mutex);
}
void portal_print(const char *s) {
    if (!s) return;
    fputs(s, stdout);
    fflush(stdout);
    append(s);
}
void portal_println(const char *s) {
    portal_print(s ? s : "");
    portal_print("\n");
}
void portal_monitor_read(char *out, size_t cap) {
    if (!out || !cap) return;
    init_mutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) { out[0] = 0; return; }
    size_t n = used < cap - 1 ? used : cap - 1;
    memcpy(out, buf + used - n, n);
    out[n] = 0;
    xSemaphoreGive(mutex);
}
void portal_monitor_clear(void) {
    init_mutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) return;
    used = 0; buf[0] = 0;
    xSemaphoreGive(mutex);
}
void portal_led_enable(int gpio) {
    if (gpio < 0 || gpio >= GPIO_NUM_MAX || !GPIO_IS_VALID_OUTPUT_GPIO(gpio)) return;
    gpio_config_t c = { .pin_bit_mask = 1ULL << gpio, .mode = GPIO_MODE_OUTPUT };
    if (gpio_config(&c) == ESP_OK) { led = gpio; portal_led_update(); }
}
void portal_led_disable(void) {
    if (led >= 0) gpio_set_level((gpio_num_t)led, 0);
    led = -1;
}
void portal_led_update(void) {
    if (led >= 0) gpio_set_level((gpio_num_t)led, wifi_esta_conectado() ? 1 : 0);
}
