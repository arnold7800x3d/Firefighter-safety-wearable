/* 
  firmware for the firefighter safety wearable.
*/

// libraries
#include <DHT.h>
#include "secrets.h"

// symbolic constants
#define DHTPIN 6
#define DHTTYPE DHT22

// objects
DHT dht(DHTPIN, DHTTYPE);

// variables
float humidity;
float temperature;

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
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
