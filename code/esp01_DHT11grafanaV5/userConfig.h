#ifndef USERCONFIG_H
#define USERCONFIG_H

#include <Arduino.h>

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
// Measurement name that will appear as a table in InfluxDB
// Example: "living_room" or "outdoor_sensor"
#define SENSOR_NAME "your-sensor"

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
// WiFi network credentials
// SSID: Your WiFi network name
// Password: Your WiFi network password
const char* ssid = "your-ssid";
const char* password = "your-password";

// WiFi connection timeout (seconds)
// Increase if you have poor signal or slow router
#define WIFI_CONNECT_TIMEOUT 10

// ============================================================================
// INFLUXDB CONFIGURATION
// ============================================================================
// InfluxDB server URL with port
// For local network: http://192.168.x.x:8086
// For remote server: https://your-domain.com (use https when possible)
#define INFLUXDB_URL "http://your-influxdb-server:8086"

// Database name (bucket in InfluxDB 2.0)
// Create this in InfluxDB UI at: Buckets section
#define INFLUXDB_DB_NAME "your-db-name"

// API Token for authentication
// Generate at: InfluxDB UI > Data > API Tokens > Generate API Token
// Select "Read/Write" scope for your bucket
#define INFLUXDB_TOKEN "your-db-token"

// Organization name (visible in InfluxDB UI)
// Example: "home" or "monitoring"
#define INFLUXDB_ORG "your-org"

// ============================================================================
// VALIDATION NOTES
// ============================================================================
// Ensure:
// 1. WiFi credentials are correct (case-sensitive)
// 2. INFLUXDB_URL is reachable from your network
// 3. INFLUXDB_TOKEN has read/write permissions for the bucket
// 4. SENSOR_NAME contains only alphanumeric characters and underscores
//    (no spaces or special characters except underscore)
// 5. MEASUREMENT_DELAY_MINUTES (in main .ino) is at least 1 minute
//    to avoid overwhelming InfluxDB with requests

#endif