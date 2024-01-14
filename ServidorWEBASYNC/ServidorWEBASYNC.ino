/***
 * Autor: Marcelo Paz
 * Fecha: 14/01/2024
 * Descripcion: Codigo para el control de un robot.
 * Firma:
  elnube      _..----.._    _
            .'  .--.    "-.(0)_
'-.__.-'"'=:|   ,  _)_ \__ . c\'-..
             '''------'---''---'-"
*/
#include "SPIFFS.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>
  
// --------------------------------------------------------------------------------------------------------------------------------------------
// Credenciales para WiFi
#include "data.h"
WiFiMulti wifiMulti;
const char* mDNS = "robotown";

// Servidores
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Pines ServoMotores
const int servoPin1 = 25;
const int servoPin2 = 26;
Servo servoM1;
Servo servoM2;

// Pines Sensor de linea
int pinLedIR = 18;
int sensorPins[8] = {19, 23, 5, 13, 12, 14, 27, 16};
unsigned long tiempoInicial;
unsigned long tiempoActual;

// Matriz para guardar ArUco
int arucoEscaneado[4][4] = {
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0}
};

// Informacion ArUcos
const int ArUcos[14][4][4] = {
  {
    {0, 1, 0, 0},
    {1, 0, 1, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 1}
  }, // 0
  {
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 1, 1, 1},
    {0, 1, 0, 1}
  }, // 1
  {
    {1, 1, 0, 0},
    {1, 1, 0, 0},
    {1, 1, 0, 1},
    {0, 0, 1, 0}
  }, // 2
  {
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {1, 0, 1, 1},
    {1, 0, 0, 1}
  }, // 3
  {
    {1, 0, 1, 0},
    {1, 0, 1, 1},
    {0, 1, 1, 0},
    {0, 0, 0, 1}
  }, // 4
  {
    {1, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 1, 0}
  }, // 5
  {
    {0, 1, 1, 0},
    {0, 0, 0, 1},
    {1, 1, 0, 1},
    {0, 0, 0, 1}
  }, // 6
  {
    {0, 0, 1, 1},
    {1, 0, 1, 1},
    {0, 0, 0, 0},
    {1, 1, 0, 1}
  }, // 7
  {
    {0, 0, 0, 0},
    {0, 0, 0, 1},
    {0, 0, 1, 0},
    {0, 1, 0, 1}
  }, // 8
  {
    {0, 0, 1, 1},
    {0, 0, 0, 0},
    {1, 0, 1, 0},
    {1, 0, 0, 1}
  }, // 9
  {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {1, 1, 1, 0}
  }, // 10
  {
    {1, 1, 1, 0},
    {1, 1, 1, 0},
    {0, 1, 0, 1},
    {1, 0, 0, 0}
  }, // 11
  {
    {1, 1, 1, 1},
    {0, 0, 0, 1},
    {0, 1, 0, 0},
    {1, 0, 0, 0}
  }, // 12
  {
    {1, 1, 0, 1},
    {0, 1, 0, 1},
    {1, 1, 1, 1},
    {0, 0, 0, 0}
  }, // 13
};

// --------------------------------------------------FUNCIONES--ARUCO-------------------------------------------------------------------------------
// Funciones Sensor de linea
void escanear() {
  return;
}

void analizar( ) {
  return;
}

// --------------------------------------------FUNCIONES--SERVOMOTORES------------------------------------------------------------------------------
void avanzar(int tiempo) {
  Serial.println("Avanzar");
  servoM1.write(180);
  servoM2.write(0);
  delay(tiempo);
}

void derecha(int tiempo) {
  Serial.println("Derecha");
  servoM1.write(180);
  servoM2.write(180);
  delay(tiempo);
}

void izquierda(int tiempo) {
  Serial.println("Izquierda");
  servoM1.write(0);
  servoM2.write(0);
  delay(tiempo);
}

void retroceder(int tiempo) {
  Serial.println("Retroceder");
  servoM1.write(0);
  servoM2.write(180);
  delay(tiempo);
}

void detener(){
  Serial.println("Detener");
  servoM1.write(90);
  servoM2.write(90);
}

// --------------------------------------------FUNCION--COMUNICACION--WEMOS--TO--PYTHON-------------------------------------------------------------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from %s url: %s\n", num, webSocket.remoteIP(num).toString().c_str(), payload);
      break;
    
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      String msg = String((char *)(payload));
      
      char* token;
      char* rest = (char*)payload;

      int tiempo;
      String instruccion;
      int i = 0;
      while ((token = strtok_r(rest, " ", &rest))){
          // token ahora contiene una parte del payload separada por espacios
          Serial.println(token);
          if (i == 0) {  // Si es el primer token, lo guardamos como una instruccion
              instruccion = token;
          }
          if (i == 1) {  // Si es el segundo token, lo guardamos como tiempo
              tiempo = atoi(token);  // Convertir el token a un entero
          }
          i++;
      }
      Serial.println(tiempo);
      if (instruccion.equalsIgnoreCase("avanzar"))
      {
        avanzar(tiempo);
        detener();
      }
      else if (instruccion.equalsIgnoreCase("retroceder"))
      {
        retroceder(tiempo);
        detener();
      }
      else if (instruccion.equalsIgnoreCase("derecha"))
      {
        derecha(tiempo);
        detener();
      }
      else if (instruccion.equalsIgnoreCase("izquierda"))
      {
        izquierda(tiempo);
        detener();
      }
      break;
  }
}

// --------------------------------------------FUNCION--SETUP---------------------------------------------------------------------------------------
void setup() {
  // Comunicacion Serial
  Serial.begin(115200);
  
  // ServoMotores
  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);

  // Memoria Interna
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Conexión WiFi
  wifiMulti.addAP(ssid_1, password_1);
  wifiMulti.addAP(ssid_2, password_2);
  wifiMulti.addAP(ssid_3, password_3);

  WiFi.mode(WIFI_STA);
  Serial.print("Conectando a Wifi ");
  while (wifiMulti.run(3000) != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado");
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Inicio mDNS
  if (!MDNS.begin("robotown")) {
    Serial.println("Error configurando mDNS!");
    return;
  }
  Serial.print("mDNS: ");
  Serial.print(mDNS);
  Serial.println(" configurado");
  Serial.print("Ingresa al link: ");
  Serial.print(mDNS);
  Serial.println(".local");
  Serial.println("¡¡NO SIEMPRE FUNCIONA EL mDNS, DEPENDE DEL PROVEEDOR!!");
  MDNS.addService("http", "tcp", 80);

  // Servidor Web Asincrono
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/imagen.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/imagen.jpg", String(), false);
  });
  
  server.on("/IzquierdaN", HTTP_GET, [](AsyncWebServerRequest * request) {
    izquierda(700);
    detener();
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Avanzar", HTTP_GET, [](AsyncWebServerRequest * request) {
    avanzar(1000);
    request->send(SPIFFS, "/Control.html", String(), false);
  });
  
  server.on("/DerechaN", HTTP_GET, [](AsyncWebServerRequest * request) {
    derecha(1050);
    detener();
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Izquierda", HTTP_GET, [](AsyncWebServerRequest * request) {
    izquierda(1000);
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Detener", HTTP_GET, [](AsyncWebServerRequest * request) {
    detener();
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Derecha", HTTP_GET, [](AsyncWebServerRequest * request) {
    derecha(1000);
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Retroceder", HTTP_GET, [](AsyncWebServerRequest * request) {
    retroceder(1000);
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  server.on("/Enviar", HTTP_POST, [](AsyncWebServerRequest * request) {
    if(request->hasParam("rutaText", true))
    {
      String msgRuta = request->getParam("rutaText", true)->value();
      Serial.print("Ruta: ");
      Serial.println(msgRuta);
      
      // Obtener la dirección IP del cliente
      IPAddress remote_ip = request->client()->remoteIP();
      Serial.print("Cliente IP: ");
      Serial.println(remote_ip);
  
      // Enviar el mensaje al cliente correspondiente
      for(uint8_t i = 0; i < webSocket.connectedClients(); i++) {
        IPAddress client_ip = webSocket.remoteIP(i);
        if(client_ip == remote_ip) {
          webSocket.sendTXT(i, msgRuta.c_str());
          break;
        }
      }
    }
    request->send(SPIFFS, "/Control.html", String(), false);
  });

  // Inicio del servidor
  server.begin();

  // Inicio del websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Se inician los pines del sensor como entrada de datos
  for (int i = 0; i < 8; i++) 
  {
    pinMode(sensorPins[i], INPUT);
  }

  // Se inicia el IR del sensor como salida en alto
  pinMode(pinLedIR, OUTPUT);
  digitalWrite(pinLedIR, HIGH);

  // Se inicia un tiempo
  tiempoInicial = millis();
}

// --------------------------------------------FUNCION--LOOP----------------------------------------------------------------------------------------
void loop(){
  tiempoActual = millis();
  // Agregar logica de los sensores de linea
  webSocket.loop();
}