#ifndef WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_wifi.h"

void inicializar_wifi(void);
void iniciar_wifi_ap(void);
void escanear_redes_wifi(void);

esp_err_t conectar_wifi_sta(
    const char *ssid,
    const char *password);

void limpiar_wifi_sta(void);
esp_err_t wifi_entrar_modo_configuracion(void);

bool wifi_esta_conectado(void);
bool wifi_ap_esta_activo(void);

const char *wifi_obtener_ssid(void);
const char *wifi_obtener_ip(void);

uint16_t wifi_obtener_cantidad_redes(void);

const wifi_ap_record_t *wifi_obtener_redes(void);

#ifdef __cplusplus
}
#endif
#endif
