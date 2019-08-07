## Demonstrator

A project that connect an Nordic Thingy:52 with an ESP32 mcu by BUS I2C for the sensors (Temperature, Humidity, Pressure and Color).
Capable of the sending the data by Mqtt using Json with cJSON, so handle connection to wifi changable with bluetooth using the given APP by BLE (possible to config Wifi, Mqtt and Sensors).

A LCD is available to display data, state, historic or logs (only when connected by BLE or authorized by Mqtt).
Also buttons to change the UI and leds to indicate the current state with diffenrets colors.

And finally, capable of updating by requesting with Mqtt, to download and changing with the given version.

##

### What sensors are we handling ?

 - Sensor Humidity/Temperature HTS221
 - Sensor Pressure/Temperature LPS22HB
 - Sensor Color RGBC BH1745NUCl

##
### Authors
 * [Raphael-G](https://github.com/Clemon-R)