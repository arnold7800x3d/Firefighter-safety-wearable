/* 
  firmware for the firefighter safety wearable.
*/

// libraries
#include <DHT.h>
#include <PubSubClient.h>
#include "secrets.h"

// symbolic constants
#define DHTPIN 6
#define DHTTYPE DHT22

// objects
DHT dht(DHTPIN, DHTTYPE);

PubSubClient mqttClient(secureClient);

// variables
float humidity;
float temperature;

/*
  credentials
*/ 

// gsm credentials
const char PIN_NUMBER[] = SECRET_PIN;
const char GPRS_APN[] = SECRET_GPRS_APN;
const char GPRS_LOGIN[] = SECRET_GPRS_LOGIN;
const char GPRS_PASSWORD[] = SECRET_GPRS_PASS;

// mqtt credentials
const char* mqttServer = SECRET_MQTT_SERVER;
const int mqttPort = SECRET_MQTT_PORT;
const char* mqttUser = SECRET_MQTT_USER;
const char* mqttPass = SECRET_MQTT_PASSWORD;

// function for publishing sensor data over mqtt
void publishData(String temp, String hum, String long_i, String lat, String bpm){
  String tempPayload = "{\"cipher\":\"" + temp + "\"}";
  String humPayload = "{\"cipher\":\"" + hum + "\"}";
  String longPayload = "{\"cipher\":\"" + long_i + "\"}";
  String latPayload = "{\"cipher\":\"" + lat + "\"}";
  String bpmPayload = "{\"cipher\":\"" + bmp + "\"}";

  mqttClient.publish("firefighter_wearable/temperature", tempPayload.c_str());
  mqttClient.publish("firefighter_wearable/humidity", humPayload.c_str());
  mqttClient.publish("firefighter_wearable/longitude", longPayload.c_str());
  mqttClient.publish("firefighter_wearable/latitude", latPayload.c_str());
  mqttClient.publish("firefighter_wearable/heatbeat", bpm.c_str());
}

// function for mqtt connection
bool connectMQTT(){
  Serial.println(F("Connecting to MQTT"));
  if(mqttClient.connect(mqttClientID, mqttUser, mqttPass)){
    Serial.println(F("MQTT connected"));
    return true;
  } else {
    Serial.println("MQTT connection failed. State: ");
    Serial.println(mqttClient.state());
    return false;
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  Serial.println(F("Setting up MQTT client"));
  secureClient.setCACert(ROOT_CA_CERT);
  mqttClient.setServer(mqttServer, mqttPort);
}

void loop() {
  if(!mqttClient.connected()){
    Serial.println(F("MQTT disconnected. Reconnecting"));
    connectMQTT();
  }

  mqttClient.loop();
  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)){
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  delay(1000);
}
