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
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// objects
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);
PulseSensorPlayground pulseSensor;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// variables
const int errorPin = 2;
const int infoPin = 4;
const int pulsePin = 34;
const int arraySize = 5;
int threshold = 550;
int sensorReadings[arraySize];
int readingSum = 0;
int validReadings = 0;

unsigned long lastReadingTime = 0;
unsigned long readingInterval = 2000;

// credentials
const char* mqttServer = SECRET_MQTT_SERVER;
const int mqttPort = SECRET_MQTT_PORT;
const char* mqttUser = SECRET_MQTT_USER;
const char* mqttPass = SECRET_MQTT_PASSWORD;

/*
 functions to indicate led alerts
*/
void publishAlert() {
  digitalWrite(infoPin, HIGH);
  delay(500);
  digitalWrite(infoPin, LOW);
}

void sensorErrorAlert() {
  digitalWrite(errorPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);
}

void networkInfoAlert() {
  digitalWrite(errorPin, HIGH);
  delay(500);
  digitalWrite(errorPin, LOW);
}

void infoAlert() {
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
void connectWiFi() {
  unsigned long wifiTimeout = 5000;
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < wifiTimeout) {
    networkInfoAlert();
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    infoAlert();
    Serial.print("\nConnected to WiFi network: ");
    Serial.println(SECRET_SSID);
  } else {
    networkInfoAlert();
    Serial.println("\nFailed to connect to the WiFi network.");
  }
}

// function to connect to mqtt broker
void connectMQTT() {
  secureClient.setCACert(ROOT_CA_CERT);
  mqttClient.setServer(mqttServer, mqttPort);
  String clientID = "M4K3R5P4C3-" + String(WiFi.macAddress());

  while (!mqttClient.connected()) {
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
void publishData(float humidity, float temperature, int bpm, float latitude, float longitude, float speed) {
  String payload = "{\"humidity\":" + String(humidity, 1) + ",\"temperature\":" + String(temperature, 1) + ",\"bpm\":" + String(bpm) + ",\"latitude\":" + String(latitude) + ",\"longitude\":" + String(longitude) + ",\"speed\":" + String(speed) + "}";

  mqttClient.publish("firefighterWearable/data", payload.c_str());
}

// function to feed serial gps data to tinyGPS++ object
void feedGPS(){
  while(gpsSerial.available()){
    gps.encode(gpsSerial.read());
  }
}

void setup() {
  Serial.begin(115200);

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  pulseSensor.analogInput(pulsePin);
  pulseSensor.setThreshold(threshold);
  if (pulseSensor.begin()) {
    Serial.println("The pulse sensor object has successfully been created");
    delay(2000);
  } else {
    Serial.println("Error in creating the pulse sensor object");
    while (1)
      ;
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

  feedGPS();

  if (millis() - lastReadingTime >= readingInterval) {
    lastReadingTime = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    float latitude  = gps.location.isValid() ? gps.location.lat() : 0.0;
    float longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
    float speed     = gps.speed.isValid()    ? gps.speed.mps()    : 0.0;

    int bpm = pulseSensor.getBeatsPerMinute();
    bool pulseOk = (bpm > 40 && bpm < 220);

    // error checking for sensors
    if (isnan(humidity) || isnan(temperature) || !pulseOk || !gps.location.isValid()) {
      sensorErrorAlert();
      Serial.println("Error! One or more sensors are not capturing valid data:");
      if (isnan(humidity))    Serial.println("  - Humidity sensor error");
      if (isnan(temperature)) Serial.println("  - Temperature sensor error");
      if (!pulseOk)           Serial.println("  - Pulse sensor error (BPM out of range or no signal)");
      if (!gps.location.isValid()) Serial.println("  - GPS: no valid location fix yet");

    } else {
      // accumulate valid BPM readings
      if (validReadings < arraySize) {
        sensorReadings[validReadings] = bpm;
        readingSum += bpm;
        validReadings++;
      } else {
        // reset average once array is full
        readingSum = 0;
        validReadings = 0;
      }

      int averageBpm = (validReadings > 0) ? (readingSum / validReadings) : bpm;

      // display all sensor data
      Serial.println("*_*_*_*_*_*_ Sensor Readings *_*_*_*_*_*_");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");

      Serial.print("BPM: ");
      Serial.println(bpm);

      Serial.print("Average BPM (last ");
      Serial.print(validReadings);
      Serial.print(" readings): ");
      Serial.println(averageBpm);

      Serial.print("Latitude: ");
      Serial.println(latitude, 6);

      Serial.print("Longitude: ");
      Serial.println(longitude, 6);

      Serial.print("Speed (m/s): ");
      Serial.println(speed, 2);
      Serial.println("*_*_*_*_*_*_*_*_*_*_*_*_");

      // publish data to broker
      publishData(humidity, temperature, averageBpm, latitude, longitude, speed);
      publishAlert();
    }
  }
}
