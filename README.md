# Wifi-temp-hum-sensor
## About
The goal of the project is to connect a DHT-11 Sensor to the local network. Store its data and plot the data in some nice graphs. All parts i used for the project itself i had laying round so its quite simple structured.

## Content
- [Features & Goals](#features)
- [Description](#description)
- [Circuit](#circuit)
- [grafana and raspberrypi](#grafana-and-raspberrypi)
- [pictures](#pictures)

### Features
- RaspberryPi 4
- InfluxDB
- Grafana
- DHT-11 (or BME-280 etc.)

### Goals
- Measure Data
- send with MQTT
- store in Database (Influxdb)
- display plots in grafana

## Description
This is just a simple Project to connect a DHT-11 Sensor to your local network, sends its data and plot these values with Grafana. The concept should work with almost any sensor, as sending data with MQTT is quite simple. In my case I’m using a ESP-8266-01 because it’s a pretty small package. <br>
In the current setup it measures temperature and humidity, optionally other sensors could be connected too. Connecting a BMx-280 sensor for example should be possible too to measure pressure as well. In this case the code must be adjusted of course, this will probably be added later. <br>
The circuit is supplied by a conventional 12V power supply I had lying around this would be a too high Voltage source for our 3.3V Microcontroller I use a buck-converter. <br>
<br>
To build this project there is some asumed knowledge necessary, like basic Linux or programming Languages (Python, C++). For beginners i will link some helpful tutorials which illustrate the configuration steps in detail, whenever i found a good one.

## Circuit
Fritzing is pretty useful for smaller projects. Youe can draw nice sketches of your circuits and export gerber files to print pcb somewhere. <br>
The Buck-converter needs to bee adjusted with a Multimeter to *3.3V* depending on your input Voltage. To supply the circuit an old *5V* micro-USB phone charger is fine, i use an old 5V adapter with a coaxial power connector so i simply switched the micro-USB connector to a fitting coaxial. <br>

![circuit](/docs/circuit.png "circuit")

![circuitraw](/docs/circuitraw.png "circuit")

<br>
The prototype version i did on breadboards, but they are impractical. So I soldered the pcb layout on simple hole grid plates (*3x7cm*) and inserted a fuse on the positive incomming wire because of security reasons. <br>

![circuitpcb](/docs/circuitpcb.png "circuit")

## Grafana and Raspberrypi
### General
Personally, I am currently using a [**RaspberryPi 4B**](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/) with *4GB* Ram on which also a [*VPN*](https://www.pivpn.io/) (**wireguard**) and [*Pihole*](https://pi-hole.net/) are installed. Thanks to the [*VPN*](https://www.pivpn.io/), I can also access the data on the go. I won't go into the installation any further, but it is relatively simple and well described in numerous tutorials. <br>Altough a VPN is not necessary for basic operation. Some basic things are important to know:
- SSid (Netzwerkname): EXAMPLE-XYZ
- Password: passwordXYZ
- IP or hostname of the RaspberryPi (IP statically assigned)
For easier configuration either use screen, ssh, teamviewer, vnc viewer ... to work with the Raspberrypi. Personally i use ssh and vnc, these are easy to install and intuitive to use. <br>

### InfluxDB
InfluxDB will be our database to store the gathered data in. 
If influx has not yet been installed, here are useful instructions [(pimylifeup)](https://pimylifeup.com/raspberry-pi-influxdb/).
<br>

To set up the database, we first start by creating a user. To do this, InfluxDB is started by entering the following line in the terminal.
```
influx
```
Influx should now have started, now enter further into the interface:
```
CREATE USER "username" WITH PASSWORD "password"
CREATE DATABASE "database"
GRANT ALL ON "username" TO "database"
```
This ***username*** as well as ***password*** and the ***database*** must be filled in in the code of the microcontrollers.
<br>
To get out of this "interface" you simply have to enter the command **exit** and you are back in the terminal.
```
exit
```

### Grafana
![grafana dashboard example](/docs/pictures/placeholder.png "placeholder") <br>

Again, there are very good [tutorials](https://grafana.com/tutorials/install-grafana-on-raspberry-pi/) that you can fall back on, so I won't go into more detail about the installation process. Grafana can be used to display the collected data in beautiful plots. Thus, a monitoring of the moisture values of the plants is very easy. <br>

Once Grafana is installed, the [json](/grafana_dashboard/bewaeMonitor.json) export can be imported via the web interface. <br>

Under 'Configuration' you now have to enter the 'Datasources'. Here you have to make sure to use the same databases that you create and in which the data is stored. In my case, that would be e.B ***main*** for all irrigation data. As well as ***pidb*** for the CPU temperature on the RaspberryPi which is included in the import, as this is purely optional and has nothing to do with irrigation, I will not go into it further. The panels when they are not in use can be easily removed. <br>

![Datasource configuration](/docs/pictures/datasources.png "Datasource configuration example") <br>

### MQTT&Python

#### MQTT
**MQTT** (Message Queuing Telemetry Transport) Is an open network protocol for machine-to-machine communication (M2M) that enables the transmission of telemetry data in the form of messages between devices. How it works [(link)](http://www.steves-internet-guide.com/mqtt-works/). <br>
The messages sent consist of topic and payload. Topics are used for simple assignment and have a fixed structure in my case:
```
#topic:
exampletopic=home/location/measurement
```
In addition to this topic, the data is appended in the payload, the content as a string.
```
#example topics to publish data on

  humidity_topic_sens2 = "home/sens2/humidity";
  #bme280 sensor read

  temperature_topic_sens2 = "home/sens2/temperature";
  #bme280 sensor read

```

#### Installation
Again, there is a [link](https://pimylifeup.com/raspberry-pi-mosquitto-mqtt-server/).
Username and password must be adjusted in all places in the code.

#### Python
In order to be able to write or read the data sent via [*MQTT*](#mqtt) to the database, the Python script [MQTTInfluxDBBridge3.py](/code/pi_scripts/MQTTInfluxDBBridge3.py) is used. The script itself comes from a [Tutorial](https://diyi0t.com/visualize-mqtt-data-with-influxdb-and-grafana/) and has been adapted to adapt it to the requirements in my project. The Python code can be started with the shell script [launcher1.sh](/code/pi_scripts/launcher1.sh) automatically with crontab at each boot process. Since the Pi needs a certain amount of time to start everything without errors, I delay the start of the script by *20* seconds. <br>
To avoid errors, only **int** values should be sent via [*MQTT*](#mqtt) (*2* **byte**), the data type **int** is large on [*Arduino Nano*](https://store.arduino.cc/products/arduino-nano) *2* **byte**. <br>
```
sudo crontab -e
```
Open cron with any editor to enter the desired programs. As an example:
```
@reboot /path/file.sh
```
Additional [information](https://pimylifeup.com/cron-jobs-and-crontab/) to cron.


## Pictures
