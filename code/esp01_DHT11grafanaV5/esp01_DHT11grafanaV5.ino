#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <Arduino.h>
#include <Wire.h>
#include "DHT.h"
#include "userConfig.h"

// Define DEBUG uncomment to stop Serial output
#define DEBUG

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
  #ifdef DEBUG
  Serial.println("Connecting to Wifi!");
  #endif
  // Wait for WiFi connection to be established or timeout after 10 seconds
  while (WiFi.status() != WL_CONNECTED && tries < 10) {
    delay(1000);
    #ifdef DEBUG
    Serial.print(".");
    #endif
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    #ifdef DEBUG
    Serial.println();
    Serial.print("Wifi Connection established! Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    #endif
    return true; // Return true if WiFi connection established
  } else {
    #ifdef DEBUG
    Serial.println();
    Serial.println("Wifi Connection failed! Check SSID, password, and signal strength.");
    #endif
    return false; // Return false if failed to connect after 10 tries
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

bool readSensorValue(DHT& dht, float &temp, float &hum) {
  #ifdef DEBUG
  Serial.println("Start measuring:");
  #endif
  float humidity;
  float temperature;
  // Take dummy measurements to stabilize the sensor readings
  for(int i=0; i<5; i++){
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    delay(1000);
  }
  
  // Take actual measurements
  float values_t[MEASUREMENTS];
  float values_h[MEASUREMENTS];
  int retries = 5;
  int i = 0;
  while ((retries > 0) && (i < MEASUREMENTS)) {
    humidity = dht.readHumidity();
    delay(1);
    temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      values_t[i] = temperature;
      values_h[i] = humidity;
      i++;
    }
    else{
      retries--;
    }
    #ifdef DEBUG
    Serial.print("Reading: "); Serial.println(i);
    Serial.print("Values: "); Serial.print(temperature); Serial.print(" - "); Serial.println(humidity);
    Serial.print("Failures: "); Serial.println(5 - retries);
    #endif
    delay(5);
  }
  
  // Return false if failed to read sensor value
  if (retries == 0){
    #ifdef DEBUG
    Serial.println("ERROR: DHT11 sensor failed to provide valid readings!");
    Serial.println("Possible causes: sensor disconnected, damaged, or requiring reset.");
    #endif
    return false;
  }

  // Calculate median temperature and humidity values and store in temp and hum variables
  temp = median(values_t, i);
  hum = median(values_h, i);
  #ifdef DEBUG
  Serial.print("Sensor readings collected: "); Serial.print(i); Serial.print("/"); Serial.println(MEASUREMENTS);
  Serial.print("Median values - Temp: "); Serial.print(temp); Serial.print("°C, Humidity: "); Serial.print(hum); Serial.println("%");
  #endif
  // Return true if sensor value read successfully
  return true;
}

// Initialize serial communication and check InfluxDB connection
void setup() {
  Serial.begin(115200);  // Set baud rate for serial communication
  #ifdef DEBUG
  Serial.print(SENSOR_NAME); Serial.println(" starting!");
  #endif
  // Check if connection to InfluxDB is successful
  if (!client.validateConnection()) {
    #ifdef DEBUG
    Serial.println("WARNING: InfluxDB connection test failed at startup!");
    Serial.print("  Error: ");
    Serial.println(client.getLastErrorMessage());
    Serial.println("  The device will continue, but data may not be stored.");
    Serial.println("  Check: INFLUXDB_URL, token, organization, and network connectivity.");
    #endif
  } else {
    #ifdef DEBUG
    Serial.println("✓ Connected to InfluxDB successfully");
    #endif
  }
  dht.begin();  // Initialize DHT sensor
}

// Main program loop
void loop() {
  
  // Check if WiFi connection is established or not
  if (!connectWifi()) {
    #ifdef DEBUG
    Serial.println("Failed to connect to WiFi");  // Print error message if connection is unsuccessful
    #endif
    return;  // Exit loop and try again later
  }

  float temperature;
  float humidity;

  // Read sensor values from DHT sensor
  if (!readSensorValue(dht, temperature, humidity)){
    #ifdef DEBUG
    Serial.println("Failed to read DHT sensor!");  // Print error message if sensor reading is unsuccessful
    #endif
  }
  else{
    // Create a new Point object to store the sensor data
    Point point(SENSOR_NAME);
    point.addField("temperature", temperature);  // Add temperature field to the Point object
    point.addField("humidity", humidity);  // Add humidity field to the Point object

    // Write the Point object to InfluxDB
    if (!client.writePoint(point)) {
      #ifdef DEBUG
      Serial.println("ERROR: Failed to write data to InfluxDB!");
      Serial.print("  Reason: ");
      Serial.println(client.getLastErrorMessage());
      Serial.println("  Check: URL, credentials, network connectivity, and InfluxDB service status.");
      #endif
    }
    else{
      #ifdef DEBUG
      Serial.print("SUCCESS: Data sent to InfluxDB (");
      Serial.print(temperature, 1);
      Serial.print("°C, ");
      Serial.print(humidity, 1);
      Serial.println("%)");
      #endif
    }
  }

  WiFi.disconnect(true);  // Disconnect from WiFi to conserve power
  WiFi.mode(WIFI_OFF);
  delay(MEASUREMENT_DELAY_MINUTES * 60UL * 1000UL);  // Wait before repeating the loop
  // ESP 8266 01 cannot wake up using sleep (at least without hardware changes)
  // ESP.deepSleep(MEASUREMENT_DELAY_MINUTES * 60UL * 1000000UL);
}