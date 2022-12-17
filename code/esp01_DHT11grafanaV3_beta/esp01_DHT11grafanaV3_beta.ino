#include <ESP8266WiFi.h> // Include the ESP8266 WiFi library
#include <PubSubClient.h> // Include the MQTT library
#include "DHT.h" // Include the DHT library
#include <Wire.h>

/*
Copyright (C) <2021>  <br-mat>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

Contact mail: matthiasbraun@gmx.at
*/

// Replace with your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Replace with your MQTT broker IP address and port
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;

// Replace with your MQTT username and password (if you have one)
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";
const char* clientID = "YOUT_MQTT_CLIENTNAME";

// Replace with your DHT pin number and type
#define DHT_PIN 2
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient espClient;
PubSubClient client(espClient);

// Wifi & MQTT constants
#define MAX_ATTEMPTS 10
#define MAX_MSG_LEN 128

// Measurements related constants
#define NUM_MEASUREMENTS 10  // Number of measurements to take

float temperature_sum = 0;  // Sum of temperature measurements
float humidity_sum = 0;  // Sum of humidity measurements

int measure_time = 15; // Default measurement interval is 15 minutes

// Set the sleep duration (in microseconds)
uint64_t sleep_duration = 1000000;  // 1 second

// Introduce helper function skelleton structure
void publishMessage();
void callback(char *topic, byte *payload, unsigned int msg_length);
String payloadToString(byte *payload, unsigned int msg_length, int max_length);

void publishMessage(const char* topic, String message) {
  // Function to publish a message to the MQTT broker, with retry logic
  int attempts = 0;
  while (!client.publish(topic, message.c_str()) && attempts < 5) {
    // Message was not successfully published, try again
    attempts++;
  }
}

void callback(char *topic, byte *payload, unsigned int msg_length) {
  // Check if the received message is on the expected topic
  if (String(topic) == "expected_topic") {
    // Convert the payload to a string
    String msg = payloadToString(payload, msg_length, MAX_MSG_LEN);
    // Convert the message string to an integer
    int new_measure_time = msg.toInt();
    // Check if the received value is within the allowed range
    if (new_measure_time >= 1 && new_measure_time <= 60) {
      // Update the measure_time variable
      measure_time = new_measure_time;
    }
  }
}

String payloadToString(byte *payload, unsigned int msg_length, int max_length) {
  // Function to convert the payload of a received MQTT message to a string
  String msg="";
  if(msg_length > max_length){ //limit msg length
    msg_length = max_length;
    #ifdef DEBUG
    Serial.print(F("Message got cut off, allowed len: "));
    Serial.println(max_length);
    #endif
  }
  for(int i=0; i< msg_length; i++){
    msg+=String((char)payload[i]);
  }
  return msg;
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");
  client.setServer(mqtt_server, mqtt_port);
  if (mqtt_user && mqtt_password) {
    client.connect(clientID, mqtt_user, mqtt_password);
  } else {
    client.connect(clientID);
  }
  client.setCallback(callback);
}


void loop() {
  // Restart WiFi and MQTT connection
  WiFi.begin(ssid, password);

  int attempts = 0;  // Initialize connection attempts counter
  while ((WiFi.status() != WL_CONNECTED) && (attempts < (int)MAX_ATTEMPTS)) {
    delay(500);
    attempts++;

    if (attempts == (int)MAX_ATTEMPTS) {
      // Connection failed after the maximum number of attempts
      // Handle the failure here
      Serial.println(F("Error: Wifi connection failed!"));
      break;
    }
  }

  client.connect(clientID, mqtt_user, mqtt_password);
  attempts = 0;  // Reset connection attempts counter
  while ((!client.connected()) && (attempts < (int)MAX_ATTEMPTS)) {
    delay(500);
    attempts++;

    if (attempts == (int)MAX_ATTEMPTS) {
      // Connection failed after the maximum number of attempts
      // Handle the failure here
      Serial.println(F("Error: mqtt connection failed!"));
      break;
    }
  }

  // Check if connection could've been established
  if(!client.connected()){
    for (int i = 0; i < NUM_MEASUREMENTS; i++) {
    // Read temperature and humidity values from the DHT sensor
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if the readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      // One or both readings are invalid, skip this iteration
      continue;
    }

    // Add the values to the sums
    temperature_sum += temperature;
    humidity_sum += humidity;
    }

    // Calculate the mean values
    float temperature_mean = temperature_sum / NUM_MEASUREMENTS;
    float humidity_mean = humidity_sum / NUM_MEASUREMENTS;

    // Reset the sums
    temperature_sum = 0;
    humidity_sum = 0;

    // Check if the temperature value is within the reasonable range
    if (temperature_mean >= -30 && temperature_mean <= 100) {
      // Publish the mean temperature value to the MQTT broker
      String temperature_str = String(temperature_mean);
      publishMessage("example_topic_1/temperature", temperature_str);
      client.loop();
    }
    
    // Check if the humidity value is within the reasonable range
    if (humidity_mean >= 0 && humidity_mean <= 100) {
      // Publish the mean humidity value to the MQTT broker
      String humidity_str = String(humidity_mean);
      publishMessage("example_topic_1/humidity", humidity_str);
      client.loop();
    }

    // Wait some seconds and listen for incoming MQTT messages
    unsigned long stamp = millis();
    while(millis() < stamp + 7500){
      // Call the loop function of the MQTT client continuously
      client.loop();
      delay(500);
    }
  }
  else{
    Serial.println(F("Error: No connection could've been established!"));
  }

  // Shut down WiFi
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  
  // Put the ESP8266 into light sleep mode for the specified interval
  // Go to sleep for the specified measure_time interval
  uint64_t sleep_duration = measure_time * 60 * 1000000;  // Convert minutes to microseconds
  esp_light_sleep_start(sleep_duration);
}
