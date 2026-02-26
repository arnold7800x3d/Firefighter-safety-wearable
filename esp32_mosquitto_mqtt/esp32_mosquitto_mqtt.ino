/*
  firmware for the firefighter safety wearable
*/

// libraries
#include <DHT.h>
#include <PubSubClient.h>
#include <PulseSensorPlayground.h>
#include "secrets.h"
#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// symbolic constants
#define DHTPIN 15
#define DHTTYPE DHT22

// objects
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);
PulseSensorPlayground pulseSensor;
TinyGPSPlus gps;

// variables
const int errorPin = 2;
const int infoPin = 4;
const int pulsePin = 13;
const int arraySize = 5;
int threshold = 550;
int sensorReadings[arraySize];
int readingSum = 0;
int validReadings = 0;

unsigned long lastBeatTime = 0;
unsigned long readingInterval = 2000;
unsigned long lastLoopTime = 0;

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

  pulseSensor.analogInput(pulsePin);
  pulseSensor.setThreshold(threshold);
  if(pulseSensor.begin()){
    Serial.println("The pulse sensor object has successfully been created");
    delay(2000);
  } else {
    Serial.println("Error in creating the pulse sensor object");
    while(1);
  }

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

  while(validReadings < arraySize){
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    float latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
    float longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
    float speed = gps.speed.isValid() ? gps.speed.mps() : 0.0;

    if (isnan(humidity) || isnan(temperature)) {
      sensorErrorAlert();
      Serial.println("Failed to read from DHT sensor!");
    } else {
      if(pulseSensor.sawStartOfBeat()){
        int bpm = pulseSensor.getBeatsPerMinute();

        if(bpm > 40 && bpm < 100){
          sensorReadings[validReadings] = bpm;
          Serial.print("Humidity: ");
          Serial.print(humidity);
          Serial.print(" %, Temperature: ");
          Serial.print(temperature);
          Serial.println(" °C");
          Serial.println("BPM: ");
          Serial.println(sensorReadings[validReadings]);
          readingSum += sensorReadings[validReadings];
          validReadings++;
        }
        lastBeatTime = millis();
      }
      
      if(millis() - lastBeatTime >= readingInterval){
        lastBeatTime = millis();
      }

      if(validReadings > 0){
        int averageBpm = readingSum / validReadings;
        Serial.print("The average BPM is: ");
        Serial.print(averageBpm);
      } else {
        Serial.println("No valid readings");
      }

      if (gps.location.isUpdated()){
        // latitude info
        Serial.print("Latitude: ");
        Serial.println(latitude);

        // longitude info
        Serial.print("Longitude: ");
        Serial.println(longitude);

        // speed in m/s
        Serial.print("Speed (m/s): ");
        Serial.println(speed);
      }

      // publish data to broker
      publishData(humidity, temperature);
      publishAlert();
    }
  }
  
  lastLoopTime = millis();
  delay(3000);
}
