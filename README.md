# weather station

a weather station using an esp8266 microcontroller.
Sends data over WiFi to the server where the data is stored.

## components
- ESP8266 Microcontroller
- SHT22 temperature and humidity sensor
- DPS310 temperature and air pressure sensor
- DS18B20 temperature sensor

## collected data
- temperature
- humidty
- air pressure
- battery voltage

## setup
you need to create a file called "wifi_credentials.h" following the example file located under src. It contains the wifi_credentials used by the microcontroller to connect to the wifi
