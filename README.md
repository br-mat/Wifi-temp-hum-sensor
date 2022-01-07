# Wifi-temp-hum-sensor
## About
The goal of the project is to connect a DHT-11 Sensor to the local network. Store its data and plot the data in nice graphs. All parts i used for the project itself i had laying round so its quite simple structured. <br>
<br>
I use it to monitor indoor and outdoor environmental values like temperature and humidity (or pressure). With a VPN, I can even watch the values on the go.
<br>

## Content
- [Features & Goals](#features)
- [Description](#description)
- [Circuit](#circuit)
- [Code](#code)
- [Grafana and RaspberryPi](#grafana-and-raspberrypi)
- [Pictures](#pictures)

### Goals
- Measure Data
- send with MQTT
- store in Database (Influxdb)
- display time series data with grafana

### Parts & Features
Hardware:
- RaspberryPi 4
- DHT-11
- Buck-converter
- micro USB connector (etc.)
- phone charger or other supply
- Resistors (2x *10k*, 2x *3.3k*)
- Capacitor (1x *100nF*)

Software:
- Raspian (etc.)
- InfluxDB
- Grafana
- Python (should be installed by default, probably some packages needed)
- Arduino IDE or Platformio
- PiVPN-wireguard (optional)


## Description
This is just a simple Project to connect a DHT-11 Sensor to your local network, sends its data and plot these values with Grafana. The concept should work with almost any sensor, as sending data with MQTT is quite simple. In my case I’m using a ESP-8266-01 because it’s a pretty small package. <br>
In the current setup it measures temperature and humidity, optionally other sensors could be connected too. Connecting a BMx-280 sensor for example should be possible too to measure pressure as well. In this case the code must be adjusted of course, this will probably be added later. <br>
The circuit is supplied by a conventional 12V power supply I had lying around this would be a too high Voltage source for our 3.3V Microcontroller I use a buck-converter. <br>
<br>
To build this project there is some assumed knowledge necessary, like basic Linux or programming Languages (Python, C++). For beginners i will link some helpful tutorials which illustrate the configuration steps in detail, whenever i found a good one.

## Circuit
I wanted to keep the effort manageable and beginner friendly therefore the circuit itself might not be efficient. As possible upgrade a lot energy can be saved when with good solder [skills](https://www.instructables.com/Enable-DeepSleep-on-an-ESP8266-01/) this way the phone charger can probably be dropped and a *9V* Battery is probably sufficient. <br>
<br>
Fritzing is pretty useful for smaller projects. You can draw nice sketches of your circuits and export gerber files to print pcb somewhere. <br>
The Buck-converter needs to bee adjusted with a Multimeter to *3.3V* depending on your input Voltage. To supply the circuit an old *5V* micro-USB phone charger is fine, i use an old 5V adapter with a coaxial power connector so i simply switched the micro-USB connector to a fitting coaxial. <br>

![circuit](/docs/circuit_board.png "circuit breadboard")

![circuitraw](/docs/circuit_raw.png "circuit")

<br>
The prototype version i did on breadboards, but they are impractical. So I soldered the PCB layout on simple hole grid plates ( *3x7cm* ) and inserted a fuse on the positive incoming wire because of security reasons. If you are using a module for the *DHT-11* simply remove *R1* and connect the module directly to *VOUT* of the Buck-converter. <br>

![circuitpcb](/docs/circuit_pcb.png "circuit pcb layout")

## Code
[code](/code/esp01_DHT11grafanaV1) <br>
I used Arduino IDE to program the microcontroller. Therefor just copy the folder and open it within the IDE. You have to install the Board manually if you have not already, as the ESP8266 is not contained by default. This is the case for the library of the DHT sensor as well, this time just use the library manager within the IDE. <br>

## Grafana and Raspberrypi
### General
Personally, I am currently using a [**RaspberryPi 4B**](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/) with *4GB* Ram on which also a [*VPN*](https://www.pivpn.io/) (**wireguard**) and [*Pihole*](https://pi-hole.net/) are installed. Thanks to the [*VPN*](https://www.pivpn.io/), I can also access the data on the go. I won't go into the installation any further, but it is relatively simple and well described in numerous tutorials. <br>Although a VPN is not necessary for basic operation. Some basic things are important to know:
- SSid (Netzwerkname): EXAMPLE-XYZ
- Password: passwordXYZ
- IP or hostname of the RaspberryPi (IP statically assigned)
For easier configuration either use screen, ssh, teamviewer, vnc viewer ... to work with the Raspberrypi. Personally I use ssh and vnc, these are easy to install and intuitive to use. <br>

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
To get out of this "interface" you simply must enter the command **exit** and you are back in the terminal.
```
exit
```

### MQTT & Python script
#### Installation
Again, there is a [link](https://pimylifeup.com/raspberry-pi-mosquitto-mqtt-server/).
Username and password must be adjusted in all places in the code. <br>
<br>
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
  #sensor read

  temperature_topic_sens2 = "home/sens2/temperature";
  #sensor read

```

<br>

#### Python script
In order to be able to write or read the data sent via [*MQTT*](#mqtt) to the database, the Python script [MQTTInfluxDBBridge3.py](/code/pi_scripts/MQTTInfluxDBBridge3.py) is used. The script itself comes from a [Tutorial](https://diyi0t.com/visualize-mqtt-data-with-influxdb-and-grafana/) and has been adapted to adapt it to the requirements in my projects. The Python code can be started with the shell script [launcher1.sh](/code/pi_scripts/launcher1.sh) automatically with crontab at each boot process. Since the Pi needs a certain amount of time to start everything without errors, I delay the start of the script by *20* seconds. <br>
To avoid errors, only **int** values should be sent via [*MQTT*](#mqtt) (*2* **byte**), the data type **int** is large on [*Arduino Nano*](https://store.arduino.cc/products/arduino-nano) *2* **byte**. <br>
```
sudo crontab -e
```
Open cron with any editor to enter the desired programs. As an example:
```
@reboot /path/file.sh
```
This file will then be executed at every reboot. It is possible to execute files in specific intervals, look up additional [information](https://pimylifeup.com/cron-jobs-and-crontab/) to cron.
<br>

### Grafana

Again, there are very good [tutorials](https://grafana.com/tutorials/install-grafana-on-raspberry-pi/) that you can fall back on, so I won't go into more detail about the installation process. Grafana can be used to display the collected data in nice plots. <br>

In order to use Grafana we need to configure our datasource and the dashboard. Starting with the datasource, first of all open a browser and enter (or whatever the IP of your pi's adress is): <br>
```
http://raspberrypi:3000/
```
- click *'Configuration'* on the left side
- Simply hit *'Add Datasoruce'*
- Now select *InfluxDB* as our Database

<br>
Now fill in name and password of the Database like in the example below. You have to make sure to use the same databases that you created earlier and in which the data is stored. In my case, that would be *main* in the example case. <br>

![Datasource configuration](/docs/pictures/datasources.png "Datasource configuration example") <br>

<br>

The next step is to configure a dashboard. Go to *'create'* and click on dashboard. Now you can add panels, in the panel you must use the database we configured earlier. For basic panels the default panel-configuration is quite useful and you do not need to know the query language. Fill in location and Value like in the example below and your panel is finished. All these steps are well explained in the tutorial of Grafana itself if there are any questions left. Although it's simple so I just pointed out the most important steps briefly. However, don't forget to apply and save your panel and dashboard. <br>

![dashboard configuration](/docs/pictures/dashboard.png "dashboard configuration example") <br>

## Pictures
![board](/docs/pictures/dht11wifi.jpg "board")

![plot](/docs/pictures/twodayschart.png "plot")
