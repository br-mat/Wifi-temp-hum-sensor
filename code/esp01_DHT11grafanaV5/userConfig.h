#ifndef USERCONFIG_H   // include guards to prevent multiple inclusion
#define USERCONFIG_H

#include <Arduino.h>

// Define the name of the sensor being used
#define SENSOR_NAME "your-sensor"

// Define the SSID and password of the WiFi network
const char* ssid = "your-ssid";
const char* password = "your-password";

// Define the URL and database name of the InfluxDB server
#define INFLUXDB_URL "http://your-influxdb-server:8086"
#define INFLUXDB_DB_NAME "your-db-name"
#define INFLUXDB_TOKEN "your-db-token"
#define INFLUXDB_ORG "your-org"

#endif