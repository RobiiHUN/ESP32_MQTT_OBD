#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "DHT.h"

/* ========================================================================== */
/*                      MAKRÓK, CSAK EZEKET KELL ÁLLÍTANI                     */
/* ========================================================================== */


#define WIFI_NEV "valami wifi SSID"
#define WIFI_JELSZO "jelszo a wifi-hez"

/* --------------------------- MQTT BROKER ADATAI --------------------------- */
#define MQTT_IP "ip cim ide"

/* ========================================================================== */





const char* ssid = WIFI_NEV;            // SSID
const char* password = WIFI_JELSZO;     // Jelszó
const char* mqtt_server = MQTT_IP;      // MQTT szerver IP címe
WiFiClient espClient;
PubSubClient client(espClient);





#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHTPIN 14
#define DHTTYPE DHT11   
DHT dht(DHTPIN, DHTTYPE);
#define ledPin  27
long lastMsg = 0;
char msg[50];
int value = 0;


void setup() {

    /* --------------------------- WI-FI KOMM. KEZDETE -------------------------- */
  Serial.begin(115200);
  WiFi.disconnect(true);
  delay(1000);
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(ssid, password);

    /* ------------------------- MQTT SERVER CSATLAKOZAS ------------------------ */

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


  pinMode(ledPin, OUTPUT);
  dht.begin();
  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");
}
void loop() {
  if (!client.connected()) {
    reconnect_mqtt();   // Ha nincs kapcsolat az mqtt szerverrel, újra csatlakozik
  }


  client.loop();
  long now = millis();
  /* ---------------------- DHT LEOLVASÁS ÉS ADAT KÜLDÉSE --------------------- */

  if (now - lastMsg > 10000) { // 10 másodpercenként leolvassuk a DHT11-t és elküldjük

    lastMsg = now;
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    // Temperature in Celsius
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    char strbuf[50];

    sprintf(strbuf, "{\"temperature\": %s, \"humidity\": %s}", tempString, humString);
    Serial.println(strbuf);

    //esp32 névre hallgatunk a serveren
    client.publish("esp32/dht11", strbuf);

  }
}

/* ---------------------- MQTT BEÉRKEZŐ ÜZENET KEZELÉSE --------------------- */

void callback(char* topic, byte* message, unsigned int length) {    // Ha MQTT üzenet jön
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  
  Serial.println();         //kiirjuk a soros portra az üzenetet
  if (String(topic) == "esp32/out") {
    digitalWrite(ledPin, HIGH);
    delay(1000);
    digitalWrite(ledPin, LOW);
  }
}
/* --------------------- MQTT SZERVER RECONNECT KEZELES --------------------- */

void reconnect_mqtt() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    Serial.println(millis());
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("esp32/out");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      Serial.print("RRSI: ");
      Serial.println(WiFi.RSSI());
      Serial.println();
      delay(5000);
    }
  }
}
/* --------------------------- WIFI EVENT KEZELES --------------------------- */
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}