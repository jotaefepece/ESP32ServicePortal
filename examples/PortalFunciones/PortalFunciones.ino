#include <ESP32ServicePortal.h>

// Ejemplo de referencia: todas las funciones publicas de ESP32ServicePortal v0.7.1
ESP32ServicePortal portal;
unsigned long ultimoMensaje = 0;

void setup() {
  Serial.begin(115200);

  // Inicia AP, gestion Wi-Fi y dashboard web.
  bool iniciado = portal.begin();

  if (!iniciado) {
    Serial.println("No se pudo iniciar el portal");
    return;
  }

  // Escribe tanto en el puerto serie como en el monitor web del portal.
  portal.print("ESP32ServicePortal ");
  portal.println("iniciado");

  // Sobrecargas que aceptan String de Arduino:
  portal.print(String("Ejemplo "));
  portal.println(String("de funciones"));

  // Imprime una linea vacia.
  portal.println();

  // LED GPIO convencional opcional: cambia el pin segun tu placa.
  // portal.enableLED(2);

  // Apaga/deshabilita el LED gestionado por la libreria.
  // portal.disableLED();

  // Fuerza la actualizacion del estado del LED configurado.
  // portal.refreshLED();

  // Borra el historial del monitor web.
  // portal.clearMonitor();
}

void loop() {
  if (millis() - ultimoMensaje >= 5000) {
    ultimoMensaje = millis();

    // Devuelve true cuando la interfaz STA esta conectada.
    bool conectado = portal.connected();

    // Devuelve la direccion IP y el SSID de la interfaz STA.
    String direccionIP = portal.ip();
    String nombreRed = portal.ssid();

    portal.print("Estado Wi-Fi: ");
    portal.println(conectado ? "Conectado" : "Sin conexion");

    if (conectado) {
      portal.print("SSID: ");
      portal.println(nombreRed);
      portal.print("IP: ");
      portal.println(direccionIP);
    }

    // Util si habilitaste un LED GPIO convencional.
    // portal.refreshLED();
  }
}
