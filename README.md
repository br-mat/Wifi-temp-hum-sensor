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

## Description
This is just a simple Project to connect a DHT-11 Sensor to your local network, sends its data and plot these values with Grafana. The concept should work with almost any sensor, as sending data with MQTT is quite simple. In my case I’m using a ESP-8266-01 because it’s a pretty small package. <br>
In the current setup it measures temperature and humidity, optionally other sensors could be connected too. Connecting a BMx-280 sensor for example should be possible too to measure pressure as well. In this case the code must be adjusted of course, this will probably be added later. <br>
The circuit is supplied by a conventional 12V power supply I had lying around this would be a too high Voltage source for our 3.3V Microcontroller. I use a buck-converter to bring it down to the save 3.3V. <br>
<br>
To build this project there is some assumed knowledge necessary, like basic Linux or programming Languages (Python, C++). For beginners i will link some helpful tutorials which illustrate the configuration steps in detail, whenever i found a good one.

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
- InfluxDB2
- Grafana
- Arduino IDE or Platformio
- PiVPN-wireguard (optional)

## Circuit
The circuit design prioritizes manageability and accessibility for beginners. However, it may not be the most efficient. With advanced soldering [skills](https://www.instructables.com/Enable-DeepSleep-on-an-ESP8266-01/), energy consumption could be reduced by using a 9V battery instead of a phone charger. In this case, the buck-converter may need to be changed. The buck-converter must be adjusted to 3.3V with a multimeter based on the input voltage. An old 5V micro-USB phone charger can power the circuit. I use an old 5V adapter with a coaxial power connector and switched the micro-USB connector to a fitting coaxial one.

Fritzing is a useful tool for smaller projects, it allows you to create clear sketches of your circuits and export gerber files for PCB printing.

![circuit](/docs/circuit_board.png "circuit breadboard")

![circuitraw](/docs/circuit_raw.png "circuit")

<br>
The prototype version i did on breadboards, but they are impractical. So I soldered the PCB layout on simple hole grid plates ( *3x7cm* ) and inserted a fuse on the positive incoming wire because of security reasons. <br>

![circuitpcb](/docs/circuit_pcb.png "circuit pcb layout")

## Code
[code](/code/esp01_DHT11grafanaV5) <br>
To program the microcontroller using the Arduino IDE, simply copy the project folder and open it within the IDE. If you haven't already done so, you'll need to [manually install the ESP8266 board package](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/), as it is not included in the Arduino IDE by default.
<br>
In addition to installing the board package, you'll also need to install libraries for any sensors or other components that you're using in your project. For this project, you'll need to install libraries for both the DHT sensor and InfluxDB. You can do this easily using the Library Manager within the Arduino IDE: simply search for each library and click "Install". <br>

## Grafana and Raspberrypi
### General
Personally, I am currently using a [**RaspberryPi 4B**](https://www.raspberrypi.com/products/raspberry-pi-4-model-b/) with *4GB* Ram on which also a [*VPN*](https://www.pivpn.io/) and [*Pihole*](https://pi-hole.net/) are installed. Thanks to the [*VPN*](https://www.pivpn.io/), I can also access the data on the go. I won't go into the installation any further, but it is relatively simple and well described in numerous tutorials. <br>Although a VPN is not necessary for basic operation. Some basic things are important to know:
- SSid (Netzwerkname): EXAMPLE-XYZ
- Password: passwordXYZ
- IP or hostname of the RaspberryPi (IP statically assigned)
For easier configuration either use screen, ssh, teamviewer, vnc viewer ... to work with the Raspberrypi. Personally I use ssh and vnc, these are easy to install and intuitive to use. <br>

### InfluxDB

InfluxDB 2.0 will be our database to store the gathered data in. If you haven't installed InfluxDB 2.0 yet, you can find instructions on how to do so on a Raspberry Pi [here](https://randomnerdtutorials.com/install-influxdb-2-raspberry-pi/).

To set up the database, start by creating an initial user through the InfluxDB user interface:
1. With InfluxDB running, visit `localhost:8086` in your web browser.
2. Click "Get Started" to set up your initial user.
3. Enter a username and password for your initial user and confirm the password.
4. Enter your initial organization name and bucket name.
5. Click "Continue" to finish setting up your initial user.

After setting up your initial user in InfluxDB 2.0, make sure to update your microcontroller code with the new username, password, organization name, and bucket name. You can do this by editing the [userConfig.h](/code/userConfig.h) file and entering the relevant information.

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
