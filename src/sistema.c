#include "sistema.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define TAG "TFM_SISTEMA"
void inicializar_sistema(void)
{
    ESP_LOGI(TAG, "Inicializando sistema...");

    ESP_ERROR_CHECK(
        esp_netif_init());

    ESP_LOGI(
        TAG,
        "Sistema de red inicializado.");

    esp_err_t event_err = esp_event_loop_create_default();
    if (event_err != ESP_OK && event_err != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(event_err);

    ESP_LOGI(
        TAG,
        "Sistema de eventos inicializado.");

    ESP_LOGI(
        TAG,
        "Inicializacion del sistema completada.");
}


/*
 * ==================================================
 * LED RGB
 * ==================================================
 */

void inicializar_led(void) {}
void apagar_led(void) {}

void inicializar_nvs(void)
{
    esp_err_t ret =
        nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND){

        ESP_ERROR_CHECK(
            nvs_flash_erase());

        ret =
            nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(
        TAG,
        "NVS inicializado.");
}
