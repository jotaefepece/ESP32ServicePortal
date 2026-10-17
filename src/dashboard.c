#include "dashboard.h"
#include "wifi_manager.h"
#include "portal_monitor.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static httpd_handle_t server;
static int hexval(char c) { if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1; }
static bool decode(const char *src, char *dst, size_t cap) {
    size_t j=0;
    for(size_t i=0;src[i];i++) {
        unsigned char ch=(unsigned char)src[i];
        if(ch=='%') { if(!src[i+1]||!src[i+2])return false; int a=hexval(src[i+1]),b=hexval(src[i+2]); if(a<0||b<0)return false; ch=(a<<4)|b; i+=2; }
        else if(ch=='+')ch=' ';
        if(ch==0||j+1>=cap)return false;
        dst[j++]=(char)ch;
    }
    dst[j]=0;return true;
}
static bool field(const char *body,const char *name,char *out,size_t cap) {
    size_t n=strlen(name);
    const char *p=body;
    while(*p) {
        const char *end=strchr(p,'&'); if(!end)end=p+strlen(p);
        if((size_t)(end-p)>n && strncmp(p,name,n)==0 && p[n]=='=') {
            size_t len=(size_t)(end-(p+n+1));
            if(len>255)return false;
            char encoded[256];memcpy(encoded,p+n+1,len);encoded[len]=0;
            return decode(encoded,out,cap);
        }
        if (!*end) { break; }
        p = end + 1;
    }
    return false;
}
static void escape_html(httpd_req_t *req, const char *s) {
    // Buffer HTML entities to avoid hundreds of tiny HTTP chunks.
    char out[256];
    size_t used = 0;
    for (; *s; ++s) {
        const char *entity = NULL;
        switch (*s) {
            case '&': entity = "&amp;"; break;
            case '<': entity = "&lt;"; break;
            case '>': entity = "&gt;"; break;
            case '"': entity = "&quot;"; break;
            case '\'': entity = "&#39;"; break;
            default: break;
        }
        const char *piece = entity ? entity : s;
        size_t len = entity ? strlen(entity) : 1;
        if (used + len >= sizeof(out)) {
            out[used] = 0;
            httpd_resp_sendstr_chunk(req, out);
            used = 0;
        }
        memcpy(out + used, piece, len);
        used += len;
    }
    if (used) {
        out[used] = 0;
        httpd_resp_sendstr_chunk(req, out);
    }
}
static esp_err_t home(httpd_req_t *req) {
    ESP_LOGI("SERVICE_PORTAL", "GET / recibido");
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_sendstr_chunk(req,
        "<!doctype html><html lang='es'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initia"
        "l-scale=1'><title>ESP32 Service Portal</title><style>*{box-sizing:border-box}body{margin:0;background:#f7f8fa;color"
        ":#1e293b;font:15px system-ui,-apple-system,sans-serif;line-height:1.5}main{max-width:560px;margin:36px auto;padding"
        ":0 18px}header{text-align:center;margin-bottom:18px}h1{font-size:22px;font-weight:650;margin:0}h2{font-size:16px;fo"
        "nt-weight:650;margin:0 0 14px}article,.notice{background:#fff;border:1px solid #dce2e8;border-radius:3px;padding:20"
        "px;margin:14px 0}article p{text-align:center;margin:4px 0 0}label{display:block;font-size:13px;font-weight:550;marg"
        "in:13px 0 5px}input,select,button{display:block;width:100%;font:inherit;padding:10px 12px;border-radius:3px}input,s"
        "elect{border:1px solid #b8c2ce;background:#fff;color:#1e293b}button{background:#71866B;color:#fff;border:1px solid "
        "#71866B;font-weight:600;margin-top:16px;cursor:pointer}button.secondary{background:#fff;color:#334155;border-color:"
        "#b8c2ce;margin-top:10px}.notice{background:#f0fdf4;border-color:#86c99b;text-align:center}.notice h2{color:#166534;"
        "margin-bottom:7px}.notice p{margin:7px 0;font-size:14px}.link{display:block;background:#16803d;color:white;text-dec"
        "oration:none;font-weight:600;padding:10px;border-radius:3px;margin:15px 0}.hint{color:#496455}.hidden{display:none}"
        ".action{display:block;width:100%;font:inherit;font-weight:600;padding:10px 12px;border:1px solid #b8c2ce;background:#fff;color:#334155;border-radius:3px;text-align:center;text-decoration:none;cursor:pointer}.action:hover{background:#f1f5f9}" "small{display:block;text-align:center;color:#64748b;font-size:12px;margin-top:16px}@media(max-width:600px){main{mar"
        "gin:22px auto;padding:0 12px}article,.notice{padding:16px}}</style></head><body><main><header><h1>ESP32 Service Por"
        "tal</h1></header><section id='aviso' class='notice hidden' aria-live='polite'><h2>Wi-Fi conectado</h2>"
        "<a class='link' id='acceso' href='#'>Abrir dashboard</a></section><articl"
        "e><h2>Estado STA</h2><p>"
    );
    if(wifi_esta_conectado()) {httpd_resp_sendstr_chunk(req,"Conectado a ");escape_html(req,wifi_obtener_ssid());httpd_resp_sendstr_chunk(req," | IP ");escape_html(req,wifi_obtener_ip());}
    else httpd_resp_sendstr_chunk(req,"Sin conexión");
    httpd_resp_sendstr_chunk(req,"</p></article>");
    if (wifi_esta_conectado()) {
        httpd_resp_sendstr_chunk(req,
            "<article><a class='action' href='/monitor'>Monitor web</a>"
            "<form method='post' action='/otra-red'><button class='secondary' type='submit'>Ingresar a otra red</button></form>"
            "<small>Versión 0.7.1</small></article></main>"
            "</html>");
        httpd_resp_sendstr_chunk(req,NULL);
        return ESP_OK;
    }
    httpd_resp_sendstr_chunk(req,"<article><h2>Conectar Wi-Fi</h2><form id='conexion' method='post' action='/configurar'><label>Red disponible</label><select name='ssid'>");
    const wifi_ap_record_t *nets=wifi_obtener_redes();
    for(uint16_t i=0;i<wifi_obtener_cantidad_redes();i++) {
        if(nets[i].ssid[0]==0)continue;
        char ssid[33];memcpy(ssid,nets[i].ssid,32);ssid[32]=0;
        httpd_resp_sendstr_chunk(req,"<option value=\"");escape_html(req,ssid);httpd_resp_sendstr_chunk(req,"\">");escape_html(req,ssid);httpd_resp_sendstr_chunk(req,"</option>");
    }
    httpd_resp_sendstr_chunk(req,
        "</select><label>Contraseña</label>"
        "<input name='password' type='password' maxlength='63' autocomplete='off' required>"
        "</form><form method='post' action='/escanear'><button class='secondary' type='submit'>Actualizar redes</button></form>"
        "<button type='submit' form='conexion'>Conectar</button>"
        "<small>Versión 0.7.1</small></article></main>"
        "<script>async function actualizar(){try{const r=await fetch('/estado',{cache:'no-store'});"
        "if(!r.ok)return;const d=await r.json();const n=document.getElementById('aviso');"
        "if(d.connected&&d.ip){n.classList.remove('hidden');const a=document.getElementById('acceso');"
        "a.href='http://'+d.ip+'/';a.textContent='Abrir dashboard · '+d.ip;"
        "}else{n.classList.add('hidden');}}catch(e){}}actualizar();setInterval(actualizar,2000);"
        "</script></html>");
    httpd_resp_sendstr_chunk(req,NULL);return ESP_OK;
}
static esp_err_t estado(httpd_req_t *req) {
    char json[128];
    snprintf(json, sizeof(json), "{\"connected\":%s,\"ap\":%s,\"ip\":\"%s\"}",
        wifi_esta_conectado() ? "true" : "false",
        wifi_ap_esta_activo() ? "true" : "false",
        wifi_esta_conectado() ? wifi_obtener_ip() : "");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, json);
}
static esp_err_t monitor_page(httpd_req_t *req) {
    if (!wifi_esta_conectado()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Monitor disponible solo en STA");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(req,
        "<!doctype html><html lang='es'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Monitor web</title><style>"
        "body{margin:0;background:#f7f8fa;color:#1e293b;font:15px system-ui,sans-serif}"
        "main{max-width:800px;margin:30px auto;padding:0 15px}"
        "h1{font-size:20px;font-weight:600}a{color:#71866B;text-decoration:none}"
        "pre{background:#17212b;color:#e2e8f0;padding:16px;min-height:300px;"
        "height:60vh;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;"
        "font:13px/1.5 ui-monospace,monospace;border-radius:3px}"
        ".actions{display:flex;gap:10px;align-items:center;margin:14px 0}"
        ".action{display:inline-block;border:1px solid #b8c2ce;background:#fff;color:#334155;"
        "border-radius:3px;padding:10px 12px;font:600 15px system-ui,sans-serif;"
        "text-decoration:none;cursor:pointer;text-align:center}"
        "</style></head><body><main><h1>Monitor web</h1>"
        "<div class='actions'><a class='action' href='/'>Volver a configuración</a>"
        "<button class='action' id='limpiar'>Limpiar</button></div>"
        "<pre id='salida'></pre></main><script>"
        "const out=document.getElementById('salida');"
        "async function leer(){try{const r=await fetch('/monitor/datos',{cache:'no-store'});"
        "if(!r.ok){out.textContent='Monitor no disponible';return;}"
        "const t=await r.text();"
        "const follow=out.scrollTop+out.clientHeight>=out.scrollHeight-30;"
        "if(out.textContent!==t){out.textContent=t;if(follow)out.scrollTop=out.scrollHeight;}"
        "}catch(e){}}"
        "document.getElementById('limpiar').onclick=async()=>{"
        "await fetch('/monitor/limpiar',{method:'POST'});leer();};"
        "leer();setInterval(leer,1000);"
        "</script></body></html>");
}
static esp_err_t monitor_data(httpd_req_t *req) {
    if (!wifi_esta_conectado()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "STA desconectada");
        return ESP_OK;
    }
    char data[4096];
    portal_monitor_read(data, sizeof(data));
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, data);
}
static esp_err_t monitor_clear(httpd_req_t *req) {
    if (!wifi_esta_conectado()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "STA desconectada");
        return ESP_OK;
    }
    portal_monitor_clear();
    return httpd_resp_sendstr(req, "");
}
static esp_err_t favicon(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}
static esp_err_t configure(httpd_req_t *req) {
    if(req->content_len<=0||req->content_len>512){httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Formulario invalido");return ESP_OK;}
    char body[513];int used=0;
    while(used<req->content_len){int n=httpd_req_recv(req,body+used,req->content_len-used);if(n<=0)return ESP_FAIL;used+=n;}
    body[used]=0;
    char ssid[33]={0},password[65]={0};
    field(body,"ssid",ssid,sizeof(ssid));
    if(!field(body,"password",password,sizeof(password)) || !ssid[0]) {httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"SSID o clave invalidos");return ESP_OK;}
    esp_err_t err=conectar_wifi_sta(ssid,password);
    memset(password,0,sizeof(password));
    if(err!=ESP_OK){httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Error conectando STA");return ESP_OK;}
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    return httpd_resp_sendstr(req,
        "<!doctype html><html lang='es'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Conectando</title><style>"
        "body{margin:0;background:#f7f8fa;color:#1e293b;font:15px system-ui,sans-serif}"
        "main{max-width:520px;margin:48px auto;padding:24px 18px;text-align:center}"
        "h1{font-size:20px;font-weight:600;margin:0 0 18px}"
        "p{margin:12px 0;color:#64748b}"
        "a{display:block;background:#71866B;color:white;text-decoration:none;font-weight:600;"
        "padding:11px 14px;border-radius:3px;margin:18px auto;max-width:300px}"
        "</style></head><body><main><h1 id='mensaje'>Conectando a la nueva red</h1>"
        "<div id='destino'></div><p id='nota'>Conéctate a la nueva red Wi-Fi.</p>"
        "<script>async function comprobar(){try{const r=await fetch('/estado',{cache:'no-store'});"
        "if(!r.ok)return;const d=await r.json();if(d.connected&&d.ip){"
        "document.getElementById('mensaje').textContent='Wi-Fi conectado';"
        "const a=document.createElement('a');a.href='http://'+d.ip+'/';"
        "a.textContent='Abrir dashboard · '+d.ip;"
        "document.getElementById('destino').replaceChildren(a);"
        "document.getElementById('nota').textContent='Usa la nueva red Wi-Fi.';"
        "clearInterval(t);}}catch(e){}}"
        "const t=setInterval(comprobar,1500);comprobar();"
        "</script></main></body></html>");
}
static void entrar_config_task(void *arg) {
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(900));
    esp_err_t err = wifi_entrar_modo_configuracion();
    if (err != ESP_OK) ESP_LOGE("SERVICE_PORTAL", "Error modo AP: %s", esp_err_to_name(err));
    vTaskDelete(NULL);
}
static esp_err_t otra_red(httpd_req_t *req) {
    if (!wifi_esta_conectado()) {
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "STA no conectada");
        return ESP_OK;
    }
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    esp_err_t response = httpd_resp_sendstr(req,
        "<!doctype html><html lang='es'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>Otra red</title><style>body{margin:0;background:#f7f8fa;color:#1e293b;"
        "font:15px system-ui,sans-serif}main{max-width:560px;margin:36px auto;padding:18px;"
        "background:#fff;text-align:center}h1{font-size:20px}p{line-height:1.6}</style></head><body><main>"
        "<h1>Modo configuración</h1>"
        "<p>Red: <strong>ESP32-Service-Portal</strong></p>"
        "<p>Abre <strong>192.168.4.1</strong></p>"
        "</main></body></html>");
    if (response == ESP_OK) {
        if (xTaskCreate(entrar_config_task, "portal_config", 3072, NULL, 5, NULL) != pdPASS)
            ESP_LOGE("SERVICE_PORTAL", "No se pudo iniciar transicion AP");
    }
    return response;
}
static esp_err_t rescan(httpd_req_t *req) {
    escanear_redes_wifi();
    httpd_resp_set_status(req,"303 See Other");httpd_resp_set_hdr(req,"Location","/");httpd_resp_send(req,NULL,0);return ESP_OK;
}
void iniciar_dashboard(void) {
    httpd_config_t cfg=HTTPD_DEFAULT_CONFIG();
    cfg.stack_size=8192;
    cfg.max_uri_handlers=12;
    ESP_ERROR_CHECK(httpd_start(&server,&cfg));
    httpd_uri_t routes[]={
        {.uri="/",.method=HTTP_GET,.handler=home},
        {.uri="/estado",.method=HTTP_GET,.handler=estado},
        {.uri="/favicon.ico",.method=HTTP_GET,.handler=favicon},
        {.uri="/configurar",.method=HTTP_POST,.handler=configure},
        {.uri="/escanear",.method=HTTP_POST,.handler=rescan},
        {.uri="/otra-red",.method=HTTP_POST,.handler=otra_red},
        {.uri="/monitor",.method=HTTP_GET,.handler=monitor_page},
        {.uri="/monitor/datos",.method=HTTP_GET,.handler=monitor_data},
        {.uri="/monitor/limpiar",.method=HTTP_POST,.handler=monitor_clear}
    };
    for(size_t i=0;i<sizeof(routes)/sizeof(routes[0]);i++)ESP_ERROR_CHECK(httpd_register_uri_handler(server,&routes[i]));
    ESP_LOGI("SERVICE_PORTAL","Dashboard HTTP iniciado");
}
