#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <Arduino.h>
#include <Wire.h>
#include "DHT.h"
#include "userConfig.h"

// Define the pin and type of DHT sensor being used
#define DHTPIN 2
#define DHTTYPE DHT11

// Define the number of measurements to take and the delay between each measurement in minutes
#define MEASUREMENTS 15
#define MEASUREMENT_DELAY_MINUTES 10

// Create an instance of the InfluxDBClient class with the specified URL, database name and token
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_DB_NAME, INFLUXDB_TOKEN);
//client.setToken(INFLUXDB_TOKEN);

// Create an instance of the DHT class with the specified pin and type
DHT dht(DHTPIN, DHTTYPE);

// Function to connect to WiFi network
bool connectWifi() {
  int tries = 0;
  WiFi.mode(WIFI_STA); // set WiFi mode to station (client)
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wifi!");
  // Wait for WiFi connection to be established or timeout after 10 seconds
  while (WiFi.status() != WL_CONNECTED && tries < 10) {
    delay(1000);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Wifi Connection established!");
    return false; // Return false if WiFi connection established
  } else {
    Serial.println();
    Serial.println("Wifi Connection failed!");
    return true; // Return true if failed to connect after 10 tries
  }
}

// Function to calculate the median value of an array
float median(float arr[], int n) {
    std::sort(arr, arr + n);
    if (n % 2 == 0) {
        return (arr[n / 2 - 1] + arr[n / 2]) / 2;
    } else {
        return arr[n / 2];
    }
}

float readSensorValue(DHT& dht, float &temp, float &hum) {
  // Take dummy measurements to stabilize the sensor readings
  for(int i=0; i<5; i++){
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
  }
  
  // Take actual measurements
  float values_t[MEASUREMENTS];
  float values_h[MEASUREMENTS];
  int retries = 5;
  int i = 0;
  while (retries > 0 && i < MEASUREMENTS) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      values_t[i] = temperature;
      values_h[i] = humidity;
      i++;
    }
    retries--;
    delay(1);
  }
  
  // Return true if failed to read sensor value
  if (retries == 0){
    Serial.println("Measuring DHT11 failed");
    return true;
  }
  
  // Calculate median temperature and humidity values and store in temp and hum variables
  temp = median(values_t, MEASUREMENTS - (5 - retries));
  hum = median(values_h, MEASUREMENTS - (5 - retries));
  
  // Return false if sensor value read successfully
  return false;
}

// Initialize serial communication and check InfluxDB connection
void setup() {
  Serial.begin(115200);  // Set baud rate for serial communication
  Serial.println("Hello World!");
  
  // Check if connection to InfluxDB is successful
  if (!client.validateConnection()) {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  } else {
    Serial.println("Connected to InfluxDB");
  }
  dht.begin();  // Initialize DHT sensor
}

// Main program loop
void loop() {
  
  // Check if WiFi connection is established or not
  if (connectWifi()) {
    Serial.println("Failed to connect to WiFi");  // Print error message if connection is unsuccessful
    return;  // Exit loop and try again later
  }

  float temperature;
  float humidity;

  // Read sensor values from DHT sensor
  if (readSensorValue(dht, temperature, humidity)){
    Serial.println("Failed to read DHT sensor!");  // Print error message if sensor reading is unsuccessful
  }
  else{
    // Create a new Point object to store the sensor data
    Point point(SENSOR_NAME);
    point.addField("temperature", temperature);  // Add temperature field to the Point object
    point.addField("humidity", humidity);  // Add humidity field to the Point object

    // Write the Point object to InfluxDB
    if (!client.writePoint(point)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());  // Print error message if write operation is unsuccessful
    }
    else{
      Serial.println("InfluxDB points sent");
    }

    WiFi.disconnect(true);  // Disconnect from WiFi to conserve power

    delay(10 * 60 * 1000000);  // Wait for 10 minutes before repeating the loop
    // ESP 8266 01 cannot wake up using sleep (at least without hardware changes)
    // ESP.deepSleep(10 * 60 * 1000000);  // Put the ESP8266 into deep sleep mode to conserve power
  }
}