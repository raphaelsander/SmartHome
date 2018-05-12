#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <FS.h>

const char* WIFI_SSID = "uaifai";
const char* WIFI_PASSWORD = "123rep456";

const char* mqttserver = "m12.cloudmqtt.com"; //m12.cloudmqtt.com (CloudMQTT)
const uint16_t mqttport = 12059; //12059 (CloudMQTT)
const char* mqttuser = "mwxjdcnp"; //NULL para sem usuário //mwxjdcnp (CloudMQTT)
const char* mqttpass = "cmrKrMLr--Rj"; //NULL para sem senha //cmrKrMLr--Rj (CloudMQTT)
const char* mqttid = "67482172314";

const int AP_CLIENTE = D5;
const int BOT = D0;

int JUMPER_RESET = 0;
int lastButtonState = 0;
int buttonState = 0;

int statusCode;
String content;

void initPins();
void initSerial();
void initWiFi();
void initMQTT();
void initSPIFFS();
void initResetJumper();
void initServer();

WiFiClient CLIENT;

PubSubClient MQTT(CLIENT);

ESP8266WebServer server(80);
#include "data.h"

void setup(void) {
  initPins();
  initSerial();
  initWiFi();
  initMQTT();
  initSPIFFS();
  initResetJumper();
  initServer();
}

void loop() {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  reconnectWiFi();
  BotState();
  MQTT.loop();
  server.handleClient();
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
  server.on("/settingsSave.json", saveSettings);

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
  String mqttserver = server.arg("mqttserver");
  server.send(200, "text/plain", "true");
}

void initResetJumper() {
  JUMPER_RESET = digitalRead(AP_CLIENTE);
  if (JUMPER_RESET == HIGH) {
    Serial.println("Sem Jumper de RESET.");
    delay(5000);
  }
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
  Serial.println("SPIFFS: " + result);
  File configfile = SPIFFS.open("/config.json", "r");
  
  if (!configfile) {
    Serial.println("Arquivo de configuração inexistente. Criando...");
    File configfile = SPIFFS.open("/config.json", "w");
    if (!configfile) {
      Serial.println("Erro ao criar arquivo de configuração.");
    }
    configfile.println("{mqttserver: test.mosquitto.org, mqttport: 1883, mqttid: 67482172314, mqttuser: NULL, mqttpass: NULL}");
  } else {
    while(configfile.available()) {
      String line = configfile.readStringUntil('n');
      Serial.println(line);
    }
  }
  configfile.close();
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

  /*boolean result = WiFi.softAP("HomeAutomation", "00000000");
  if(result == true){
    Serial.println("Configurando AP: Ok");
  }
  else{
    Serial.println("Configurando AP: Erro!");
  }*/
  
  delay(10);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Conectando na rede: ");
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

