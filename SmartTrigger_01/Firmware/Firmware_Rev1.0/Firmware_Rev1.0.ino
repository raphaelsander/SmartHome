#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <FS.h>
#include <EEPROM.h>

/*
Wireless AP
Wireless STA
MQTT
Arquivo de configuração:
https://arduino.stackexchange.com/questions/25945/how-to-read-and-write-eeprom-in-esp8266
https://github.com/esp8266/Arduino/tree/master/libraries/EEPROM/examples
http://pedrominatel.com.br/pt/esp8266/utilizando-eeprom-do-esp8266/
Botão de restet
Requests
Web Server
mDNS => https://tttapa.github.io/ESP8266/Chap08%20-%20mDNS.html
*/

const char* WIFI_SSID = "uaifai";
const char* WIFI_PASSWORD = "123rep456";

const char* mqttserver = "m12.cloudmqtt.com"; //m12.cloudmqtt.com (CloudMQTT)
const uint16_t mqttport = 12059; //12059 (CloudMQTT)
const char* mqttuser = "mwxjdcnp"; //NULL para sem usuário //mwxjdcnp (CloudMQTT)
const char* mqttpass = "cmrKrMLr--Rj"; //NULL para sem senha //cmrKrMLr--Rj (CloudMQTT)
const char* mqttid = "67482172314";

const int AP_CLIENTE = D5;
const int BOT = D0;

int lastButtonState = 0;
int buttonState = 0;

int statusCode;
String content;
String Teste;
struct data;

void initPins();
void initSerial();
void initWiFi();
void initMQTT();
void initSPIFFS();
void initResetJumper();
void initServer();

struct {
    bool firstconfig = true;
    char ssid[20] = "";
    char pass[32] = "";
} data;

WiFiClient CLIENT;

PubSubClient MQTT(CLIENT);

ESP8266WebServer server(80);
#include "data.h"

void setup(void) {
  initPins();
  initSerial();
  EEPROM.begin(300);
  if (EEPROM.read(0) == 255) {
    initStruct();
    Serial.println(EEPROM.read(0));
  }

  if (data.firstconfig == true) {
    initWiFi();
  } else {
    initMQTT();
    initServer();
  }
}

void loop() {
  //if (!MQTT.connected()) {
    //reconnectMQTT();
  //}
  //reconnectWiFi();
  BotState();
  MQTT.loop();
  server.handleClient();
}

void initStruct() {
  
  uint addr = 1;

  
}

void initServer() {
  // HTML
  server.on ("/", indexHTML);
  server.on ("/settings.html", settingsHTML);
  server.on ("/apscan.html", apscanHTML);
  server.on ("/info.html", infoHTML);
  server.on ("/error.html", errorHTML);

  // JS
  server.on("/js/functions.js", loadFunctionsJS);
  server.on("/js/settings.js", loadSettingsJS);

  // JSON
  server.on("/config", saveSettings);

  // CSS
  server.on ("/style.css", loadStyle);

  server.begin();
}

void apscanHTML() {
  sendFile(200, "text/html", data_apscanHTML, sizeof(data_apscanHTML));
}

void settingsHTML() {
  sendFile(200, "text/html", data_settingsHTML, sizeof(data_settingsHTML));
}

void errorHTML() {
  sendFile(200, "text/html", data_errorHTML, sizeof(data_errorHTML));
}

void indexHTML() {
  sendFile(200, "text/html", data_indexHTML, sizeof(data_indexHTML));
}

void infoHTML() {
  sendFile(200, "text/html", data_infoHTML, sizeof(data_infoHTML));
}

void loadFunctionsJS() {
  sendFile(200, "text/javascript", data_js_functionsJS, sizeof(data_js_functionsJS));
}

void loadSettingsJS() {
  sendFile(200, "text/javascript", data_js_settingsJS, sizeof(data_js_settingsJS));
}

void loadStyle() {
  sendFile(200, "text/css;charset=UTF-8", data_styleCSS, sizeof(data_styleCSS));
}

void saveSettings() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  server.send(200, "text/plain", "true");
}

void BotState() {
  if (digitalRead(D0) != lastButtonState) {
    if (lastButtonState == 0) {
      digitalWrite(D3, 1);
    }
    if (lastButtonState == 1) {
      digitalWrite(D3, 0);
    }
  }

  lastButtonState = digitalRead(D0);
}

void initSPIFFS() {
  bool result = SPIFFS.begin();
  Serial.print("SPIFFS... ");
  if (result == 1) {
    Serial.println("[OK]");
  } else {
    Serial.println("[ERRO]");
  }
  File configfile = SPIFFS.open("/config.json", "r");
  
  if (!configfile) {
    Serial.print("Criando arquivo config... ");
    File configfile = SPIFFS.open("/config.json", "w");
    if (!configfile) {
      Serial.println("[ERRO]");
    } else {
      Serial.println("[OK]");
    }
    configfile.print("{mqttserver: NULL, mqttport: NULL, mqttid: NULL, mqttuser: NULL, mqttpass: NULL, ssid: NULL, pass: NULL}");
  } else {
    while(configfile.available()) {
      Teste += configfile.readStringUntil('n');
    }
  }
  configfile.close();
  Serial.println(Teste);
}

void initPins() {
  pinMode(D3, OUTPUT);
  pinMode(D5, INPUT);
  pinMode(D0, INPUT_PULLUP);
}

void initSerial() {
  delay(300);
  Serial.begin(9600);
}

void initWiFi() {
  Serial.print("Configurando AP... ");
  boolean result = WiFi.softAP("AutoConnect", "00000000");
  if(result == true) {
    Serial.println("[OK]");
  } else {
    Serial.println("[ERRO]");
  }
}


void WIFI() {
  
  delay(10);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Conectando na rede ");
  Serial.println(WIFI_SSID);
  
  while (WiFi.status() != WL_CONNECTED) {
   Serial.print(".");
   delay(1000);
  }
  
  Serial.println();
  Serial.println("Conectado na Rede " + String(WIFI_SSID));
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(mqttserver, mqttport);
  MQTT.setCallback(mqtt_callback);
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.println("Conectando ao Broker MQTT");
    if (MQTT.connect("ESP8266", mqttuser, mqttpass)) {
      Serial.println("Conectado com Sucesso ao Broker");
      MQTT.subscribe("blackcat/cont");
    } else {
      Serial.print("Falha ao Conectador, Código= ");
      Serial.println(MQTT.state());
      delay(2000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  String message;
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    message += c;
  }
  Serial.println("Tópico => " + String(topic) + " | Valor => " + String(message));
  if (message == "ON") {
    digitalWrite(D3, 1);
  } else {
    digitalWrite(D3, 0);
  }
  Serial.flush();
}

void reconnectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
}

