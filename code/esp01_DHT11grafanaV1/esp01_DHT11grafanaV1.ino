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

//MQTT CLIENT ID and TOPIC needs to be CHANGED for every INDIVIDUAL sensor

#include "PubSubClient.h" // Connect and publish to the MQTT broker
#include <Wire.h>

// Code for the ESP8266
#include <ESP8266WiFi.h> // Enables the ESP to connect to the local network (via WiFi)
#include "DHT.h"

#define DHTPIN 2     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);


// WiFi
const char* ssid = "XXXXXX";         // Your personal network SSID
const char* wifi_password = "XXXXXX"; // Your personal network password

// MQTT
const char* mqtt_server = "10.0.0.XX";  // IP of the MQTT broker
const char* humidity_topic = "home/sens1/humidity";
const char* temperature_topic = "home/sens1/temperature";
const char* mqtt_username = "XXXXX"; // MQTT username
const char* mqtt_password = "XXXXX"; // MQTT password
const char* clientID = "client_sens1"; // MQTT client ID

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 


// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT(){
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  delay(1);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);
  int tries = 0;
  // Wait until the connection has been confirmed before continuing
  while ((WiFi.status() != WL_CONNECTED) && (tries++ < 20)) {
    delay(500);
    Serial.print(" . ");
    if(tries == 10) {
      WiFi.begin(ssid, wifi_password);
      Serial.print(F(" Trying again "));
      Serial.print(tries);
      delay(2500);
    }
  }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}


void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  connect_MQTT();
  Serial.setTimeout(2000);
  delay(200);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  float pressure;
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  /*
  // MQTT can only transmit strings
  String hs="Hum: "+String((float)h)+" % ";
  String ts="Temp: "+String((float)t)+" C ";
  */
  
  // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
  if (client.publish(temperature_topic, String(t).c_str())) {
    Serial.println("Temperature sent!");
    Serial.println(t);
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println(F("Temperature failed to send. Reconnecting to MQTT Broker and trying again"));
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(temperature_topic, String(t).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = Humidity, defined at the beginning)
  if (client.publish(humidity_topic, String(h).c_str())) {
    Serial.println("Humidity sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println(F("Humidity failed to send. Reconnecting to MQTT Broker and trying again"));
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(humidity_topic, String(h).c_str());
  }
  client.disconnect();  // disconnect from the MQTT broker

  //modem sleep
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  Serial.println("WiFi is down");
  delay(595000UL); //~5 min (294)sek
 

  WiFi.forceSleepWake();
  delay(1);
}
