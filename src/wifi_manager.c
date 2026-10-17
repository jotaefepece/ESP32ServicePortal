#include "wifi_manager.h"
#include "portal_monitor.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "TFM_WIFI"
#define AP_SSID "ESP32-Service-Portal"
#define MAX_REDES_WIFI 20


/*
 * ==================================================
 * Estado interno WiFi
 * ==================================================
 */

static char wifi_ssid[33] = "";
static char wifi_password[65] = "";
static char wifi_ip[16] = "";

static volatile bool wifi_conectado = false;
static volatile bool ap_activo = true;
static volatile bool transicion_pendiente = false;
static volatile bool credenciales_guardadas = false;
static bool auto_reconectar = true;
static TickType_t proximo_intento = 0;
static bool tarea_iniciada = false;
static volatile bool escaneo_en_curso = false;

static void guardar_credenciales(void) {
    nvs_handle_t nvs;
    if (nvs_open("portal_wifi", NVS_READWRITE, &nvs) != ESP_OK) return;
    esp_err_t a = nvs_set_str(nvs, "ssid", wifi_ssid);
    esp_err_t b = nvs_set_str(nvs, "password", wifi_password);
    if (a == ESP_OK && b == ESP_OK && nvs_commit(nvs) == ESP_OK) {
        credenciales_guardadas = true;
    }
    nvs_close(nvs);
}

static void tarea_wifi(void *arg) {
    (void)arg;
    TickType_t conectado_desde = 0;
    while (true) {
        if (wifi_conectado) {
            if (transicion_pendiente) {
                if (!conectado_desde) conectado_desde = xTaskGetTickCount();
                if ((xTaskGetTickCount() - conectado_desde) >= pdMS_TO_TICKS(30000)) {
                    transicion_pendiente = false;
                    if (wifi_conectado && ap_activo) {
                        esp_err_t e = esp_wifi_set_mode(WIFI_MODE_STA);
                        if (e == ESP_OK) {
                            ap_activo = false;
                            ESP_LOGI(TAG, "AP desactivado; dashboard STA: http://%s/", wifi_ip);
                        } else ESP_LOGW(TAG, "No se pudo desactivar AP: %s", esp_err_to_name(e));
                    }
                }
            }
        } else {
            conectado_desde = 0;
            if (!ap_activo) {
                if (esp_wifi_set_mode(WIFI_MODE_APSTA) == ESP_OK) {
                    ap_activo = true;
                    ESP_LOGW(TAG, "AP de recuperacion habilitado: 192.168.4.1");
                }
            }
            if (!escaneo_en_curso && auto_reconectar && wifi_ssid[0] &&
                (int32_t)(xTaskGetTickCount() - proximo_intento) >= 0) {
                proximo_intento = xTaskGetTickCount() + pdMS_TO_TICKS(15000);
                esp_err_t e = esp_wifi_connect();
                if (e != ESP_OK) ESP_LOGW(TAG, "Reconexion: %s", esp_err_to_name(e));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void cargar_credenciales(void) {
    nvs_handle_t nvs;
    if (nvs_open("portal_wifi", NVS_READONLY, &nvs) != ESP_OK) return;
    size_t a = sizeof(wifi_ssid), b = sizeof(wifi_password);
    if (nvs_get_str(nvs, "ssid", wifi_ssid, &a) == ESP_OK &&
        nvs_get_str(nvs, "password", wifi_password, &b) == ESP_OK && wifi_ssid[0]) {
        credenciales_guardadas = true;
        ESP_LOGI(TAG, "Credenciales STA recuperadas de NVS");
    } else { wifi_ssid[0] = 0; wifi_password[0] = 0; }
    nvs_close(nvs);
}


static wifi_ap_record_t redes_wifi[MAX_REDES_WIFI];
static uint16_t cantidad_redes_wifi = 0;


/*
 * ==================================================
 * Eventos WiFi
 * ==================================================
 */

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START){

        ESP_LOGI(
            TAG,
            "STA iniciada.");

        return;
    }


    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED){

        wifi_conectado = false;
        portal_led_update();
        transicion_pendiente = false;
        wifi_ip[0] = 0;
        proximo_intento = xTaskGetTickCount() + pdMS_TO_TICKS(3000);

        wifi_event_sta_disconnected_t *evento =
            (wifi_event_sta_disconnected_t *)event_data;

        ESP_LOGW(
            TAG,
            "STA desconectada. Razon: %d",
            evento->reason);

        return;
    }


    if (event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP){

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;


        snprintf(
            wifi_ip,
            sizeof(wifi_ip),
            IPSTR,
            IP2STR(&event->ip_info.ip));


wifi_conectado = true;
        portal_led_update();
        transicion_pendiente = ap_activo;
        if (!credenciales_guardadas) guardar_credenciales();

        ESP_LOGI(
            TAG,
            "STA conectada. IP: %s",
            wifi_ip);


    }
}


/*
 * ==================================================
 * Inicializacion
 * ==================================================
 */

void inicializar_wifi(void)
{
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();


    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();


    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));


    wifi_config_t sta_config = {0};


    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_APSTA));


    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &sta_config));


    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL));


    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL));


    cargar_credenciales();
    if (wifi_ssid[0]) {
        wifi_config_t saved = {0};
        memcpy(saved.sta.ssid, wifi_ssid, strlen(wifi_ssid));
        memcpy(saved.sta.password, wifi_password, strlen(wifi_password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &saved));
    }
    ESP_LOGI(TAG, "WiFi inicializado.");
}


/*
 * ==================================================
 * Punto de acceso
 * ==================================================
 */

void iniciar_wifi_ap(void)
{
    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .password = "esp32service"
        }
    };


    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_APSTA));


    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_AP,
            &ap_config));


    ESP_ERROR_CHECK(
        esp_wifi_start());


    ap_activo = true;
    // Programar el siguiente intento antes de activar la tarea de supervision.
    proximo_intento = xTaskGetTickCount() + pdMS_TO_TICKS(15000);
    if (wifi_ssid[0]) {
        esp_err_t e = esp_wifi_connect();
        if (e == ESP_ERR_WIFI_CONN) {
            ESP_LOGW(TAG, "Conexion STA ya en curso; se espera el resultado");
        } else if (e != ESP_OK) {
            ESP_LOGW(TAG, "Inicio STA: %s", esp_err_to_name(e));
        }
    }
    if (!tarea_iniciada) {
        tarea_iniciada = true;
        xTaskCreate(tarea_wifi, "portal_wifi", 4096, NULL, 5, NULL);
    }
    ESP_LOGI(TAG, "AP iniciado: %s", AP_SSID);
}


/*
 * ==================================================
 * Escaneo de redes
 * ==================================================
 */

void escanear_redes_wifi(void)
{
    if (escaneo_en_curso) {
        ESP_LOGW(TAG, "Escaneo ya en curso");
        return;
    }
    escaneo_en_curso = true;
    ESP_LOGI(TAG, "Escaneando redes WiFi...");

    // Si la STA busca el router perdido, detener ese intento antes
    // de escanear. La tarea de reconexion espera al finalizar.
    if (!wifi_conectado && wifi_ssid[0]) {
        esp_err_t stop_err = esp_wifi_disconnect();
        if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_CONNECT) {
            ESP_LOGW(TAG, "Detener intento STA: %s", esp_err_to_name(stop_err));
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    esp_err_t scan_err = esp_wifi_scan_start(NULL, true);
    if (scan_err != ESP_OK) {
        ESP_LOGW(TAG, "Escaneo omitido: %s", esp_err_to_name(scan_err));
        escaneo_en_curso = false;
        return;
    }

    cantidad_redes_wifi =
        MAX_REDES_WIFI;


    esp_err_t records_err = esp_wifi_scan_get_ap_records(&cantidad_redes_wifi, redes_wifi);
    if (records_err != ESP_OK) {
        ESP_LOGW(TAG, "Lectura de redes: %s", esp_err_to_name(records_err));
        cantidad_redes_wifi = 0;
    }
    if (!wifi_conectado && wifi_ssid[0] && auto_reconectar) {
        proximo_intento = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
    }
    escaneo_en_curso = false;


    ESP_LOGI(
        TAG,
        "Redes encontradas: %d",
        cantidad_redes_wifi);


    for (int i = 0;
         i < cantidad_redes_wifi;
         i++){

        ESP_LOGI(
            TAG,
            "%d - %s RSSI: %d",
            i + 1,
            (char *)redes_wifi[i].ssid,
            redes_wifi[i].rssi);
    }
}


/*
 * ==================================================
 * Conexion STA
 * ==================================================
 */

esp_err_t conectar_wifi_sta(
    const char *ssid,
    const char *password)
{
    auto_reconectar = true;
    if (ssid == NULL ||
        password == NULL){

        return ESP_ERR_INVALID_ARG;
    }


    if (strlen(ssid) == 0 || strlen(ssid) > 32 || strlen(password) > 63) return ESP_ERR_INVALID_ARG;

    memset(
        wifi_ssid,
        0,
        sizeof(wifi_ssid));

    memset(
        wifi_password,
        0,
        sizeof(wifi_password));


    strncpy(
        wifi_ssid,
        ssid,
        sizeof(wifi_ssid) - 1);

    strncpy(
        wifi_password,
        password,
        sizeof(wifi_password) - 1);


    wifi_config_t sta_config = {0};


    strncpy(
        (char *)sta_config.sta.ssid,
        wifi_ssid,
        sizeof(sta_config.sta.ssid) - 1);


    strncpy(
        (char *)sta_config.sta.password,
        wifi_password,
        sizeof(sta_config.sta.password) - 1);


    // Mantener el AP mientras se prepara la conexion STA.
    if (!ap_activo) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ap_activo = true;
    }
    if (wifi_conectado) {
        esp_err_t e = esp_wifi_disconnect();
        if (e != ESP_OK) return e;
    }
    wifi_conectado = false;
    credenciales_guardadas = false;
    transicion_pendiente = false;
    proximo_intento = xTaskGetTickCount() + pdMS_TO_TICKS(15000);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_LOGI(
        TAG,
        "Conectando a red STA: %s",
        wifi_ssid);


    return esp_wifi_connect();
}


/*
 * ==================================================
 * Limpiar conexion STA
 * ==================================================
 */

void limpiar_wifi_sta(void)
{
    auto_reconectar = false;
    esp_wifi_disconnect();


    wifi_config_t config_vacia = {0};


    esp_err_t resultado =
        esp_wifi_set_config(
            WIFI_IF_STA,
            &config_vacia);


    if (resultado != ESP_OK){

        ESP_LOGW(
            TAG,
            "No se pudo limpiar STA: %s",
            esp_err_to_name(resultado));
    }


    wifi_conectado = false;
    transicion_pendiente = false;
    nvs_handle_t nvs;
    if (nvs_open("portal_wifi", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_all(nvs); nvs_commit(nvs); nvs_close(nvs);
    }
    credenciales_guardadas = false;

    memset(
        wifi_ssid,
        0,
        sizeof(wifi_ssid));

    memset(
        wifi_password,
        0,
        sizeof(wifi_password));

    memset(
        wifi_ip,
        0,
        sizeof(wifi_ip));
}


/*
 * ==================================================
 * Consultas
 * ==================================================
 */

bool wifi_esta_conectado(void)
{
    return wifi_conectado;
}


const char *wifi_obtener_ssid(void)
{
    return wifi_ssid;
}


const char *wifi_obtener_ip(void)
{
    return wifi_ip;
}


uint16_t wifi_obtener_cantidad_redes(void)
{
    return cantidad_redes_wifi;
}


const wifi_ap_record_t *wifi_obtener_redes(void)
{
    return redes_wifi;
}

bool wifi_ap_esta_activo(void) { return ap_activo; }

/* Seleccion explicita de otra red: AP permanente hasta nueva provision. */
esp_err_t wifi_entrar_modo_configuracion(void)
{
    limpiar_wifi_sta();
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err == ESP_OK) {
        ap_activo = true;
        transicion_pendiente = false;
        ESP_LOGI(TAG, "Modo configuracion solicitado: http://192.168.4.1/");
    }
    return err;
}
