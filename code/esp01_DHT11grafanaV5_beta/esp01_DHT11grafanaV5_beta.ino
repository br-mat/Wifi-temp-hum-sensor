#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT11

const char* ssid = "your-ssid";
const char* password = "your-password";

#define INFLUXDB_URL "http://your-influxdb-server:8086"
#define INFLUXDB_DB_NAME "in"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

DHT dht(DHTPIN, DHTTYPE);

bool connectWifi() {
  int tries = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tries++;
    if (tries >= 10) {
      return true;
    }
  }
  return false;
}

float readSensorValue(DHT& dht) {
  float values[20];
  int retries = 5;
  int i = 0;
  while (retries > 0 && i < 20) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      values[i] = temperature;
      i++;
    }
    retries--;
    delay(500);
  }
  if (i == 0) {
    return NAN;
  }
  float median = 0.0;
  int middle = i / 2;
  if (i % 2 == 0) {
    median = (values[middle] + values[middle - 1]) / 2.0;
  } else {
    median = values[middle];
  }
  return median;
}

void setup() {
  Serial.begin(115200);
  
  if (!client.validateConnection()) {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  } else {
    Serial.println("Connected to InfluxDB");
  }

  dht.begin();
}

void loop() {
  
  if (connectWifi()) {
    Serial.println("Failed to connect to WiFi");
    return;
  }

  float temperature = readSensorValue(dht);

  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Point point("sensor1");
  point.addField("temperature", temperature);

  if (!client.writePoint(point)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  WiFi.disconnect(true);
  
  ESP.deepSleep(10 * 60 * 1000000);
}
