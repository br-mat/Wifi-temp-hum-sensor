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

// Measure intervall
unsigned long intervall = 60*1000* 299; //299s ~ evrey 5 min

// WiFi
const char* ssid = "A1-08e52c";         // Your personal network SSID
const char* wifi_password = "mabritck"; // Your personal network password

// MQTT
const char* mqtt_server = "10.0.0.18";  // IP of the MQTT broker
const char* humidity_topic = "home/sens1/humidity";
const char* temperature_topic = "home/sens1/temperature";
const char* mqtt_username = "mbraun"; // MQTT username
const char* mqtt_password = "wdsaibuii123"; // MQTT password
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


static bool measure(float *temperature, float *humidity)
{
  static unsigned long timestamp = millis( );
  /* Measure once every four seconds. */
  if(millis() - timestamp > intervall)
  {
    *humidity = dht.readHumidity();
    *temperature = dht.readTemperature();
    if(isnan(*humidity) || isnan(*temperature))
    {
      //*temperature=t;
      //*humidity=h;
      timestamp = millis();
      return(true);
    }
  }
  return(false);
}


void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop( )
{
  float temperature;
  float humidity;

  /* Measure temperature and humidity.  If the functions returns
     true, then a measurement is available. */
  if(measure(&temperature, &humidity) == true)
  {
    WiFi.forceSleepWake();
    delay(1);
    connect_MQTT();
    Serial.setTimeout(2000);
    delay(200);
    float h = humidity; // Read temperature as Celsius (the default)
    float t = temperature;

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
    Serial.println("WiFi sleep");
  }
}
