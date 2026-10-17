#ifndef SISTEMA_H

#ifdef __cplusplus
extern "C" {
#endif
#define SISTEMA_H

void inicializar_sistema(void);
void inicializar_led(void);
void apagar_led(void);
void inicializar_nvs(void);

#ifdef __cplusplus
}
#endif
#endif
