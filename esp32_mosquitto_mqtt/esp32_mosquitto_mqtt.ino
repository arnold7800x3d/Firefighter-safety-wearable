/*
  firmware for the firefighter safety wearable
*/

// libraries
#include <DHT.h>
#include <PubSubClient.h>
#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// symbolic constants
#define DHTPIN 15
#define DHTTYPE DHT22

// objects
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// variables
const int errorPin = 2;
const int infoPin = 4;

// credentials
const char* mqttServer = SECRET_MQTT_SERVER;
const int mqttPort = SECRET_MQTT_PORT;
const char* mqttUser = SECRET_MQTT_USER;
const char* mqttPass = SECRET_MQTT_PASSWORD;

/*
 functions to indicate led alerts
*/
void publishAlert(){
  digitalWrite(infoPin, HIGH);
  delay(500);
  digitalWrite(infoPin, LOW);
}

void sensorErrorAlert(){
  digitalWrite(errorPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);
}

void networkInfoAlert() {
  digitalWrite(errorPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);
}

void infoAlert(){
  digitalWrite(infoPin, HIGH);
  delay(1000);
  digitalWrite(infoPin, LOW);
}

/*
void errorAlert(){
  digitalWrite(errorPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);
}*/

// function to connect to wifi
void connectWiFi(){
  unsigned long wifiTimeout = 5000;
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  unsigned long startAttempt = millis();

  while(WiFi.status() != WL_CONNECTED && millis() - startAttempt < wifiTimeout){
    networkInfoAlert();
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  if(WiFi.status() == WL_CONNECTED){
    infoAlert();
    Serial.print("\nConnected to WiFi network: ");
    Serial.println(SECRET_SSID);
  } else {
    networkInfoAlert();
    Serial.println("\nFailed to connect to the WiFi network.");
  }
}

// function to connect to mqtt broker
void connectMQTT(){
  secureClient.setCACert(ROOT_CA_CERT);
  mqttClient.setServer(mqttServer, mqttPort);
  String clientID = "M4K3R5P4C3-" + String(WiFi.macAddress());
  
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT connection...");

    if (mqttClient.connect(clientID.c_str(), mqttUser, mqttPass)) {
      infoAlert();
      Serial.println("Connected to Mosquitto MQTT broker");
    } else {
      networkInfoAlert();
      Serial.print("Connection failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" .Retrying in 3 seconds");
      delay(3000);
    }
  }
}

// function to publish data to the broker
void publishData(float humidity, float temperature){
  String payload = "{\"humidity\":" + String(humidity, 1) + ",\"temperature\":" + String(temperature, 1) + "}";

  mqttClient.publish("firefighterWearable/data", payload.c_str());
}

void setup() {
  Serial.begin(115200);

  pinMode(errorPin, OUTPUT);
  pinMode(infoPin, OUTPUT);

  dht.begin();

  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    sensorErrorAlert();
    Serial.println("Failed to read from DHT sensor!");
  } else {    
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    // publish data to broker
    publishData(humidity, temperature);
    publishAlert();
  }

  delay(1000);
}
