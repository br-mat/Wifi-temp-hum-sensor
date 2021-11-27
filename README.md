# Wifi-temp-hum-sensor
## About
 DHT-11 based sensor to hook up with Wifi network. Values will be sent via MQTT and displayed with grafana.

## Content
- [Features](#features)

## Features
- Measure Data
- send with MQTT
- store in Database (Influxdb)
- display plots in grafana

## Description
This is just a simple Project to connect a DHT-11 Sensor to your local network, sends its data and plot these values with Grafana. The concept should work with almost any sensor, as sending data with MQTT is quite simple. In my case I’m using a ESP-8266-01 because it’s a pretty small package. <br>
In the current setup it measures temperature and humidity, optionally other sensors could be connected too. Connecting a BMx-280 sensor for example should be possible too to measure pressure as well. In this case the code must be adjusted of course, this will probably be added later. <br>
The circuit is supplied by a conventional 12V power supply I had lying around this would be a too high Voltage source for our 3.3V Microcontroller I use a buck-converter.
