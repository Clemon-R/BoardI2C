## Demonstrator
### What is it ?

A project that connect an Nordic Thingy:52 with an ESP32 mcu by BUS I2C for the sensors (Temperature, Humidity, Pressure and Color).
Capable of the sending the data by Mqtt using Json with cJSON, so handle connection to wifi changable with bluetooth using the given APP by BLE (possible to config Wifi, Mqtt and Sensors).

A LCD is available to display data, state, historic or logs (only when connected by BLE or authorized by Mqtt).
Also buttons to change the UI and leds to indicate the current state with diffenrets colors.

And finally, capable of updating by requesting with Mqtt, to download and changing with the given version.

**INFORMATION:** By default is running on a server test mqtt

##

### Remaining problems

* If LCD is launched before the code in the while in the main, we got a white screen
* Plugging SDA and SCL without power supply in the Nordic, the ESP32 don't start(typically removing the jack)


##

### How does it work ?

When already uploaded on your ESP32 with all the required components and settings,
it automatically connect to the Wifi and after to the server Mqtt, it also calibrate the sensors and then upload the data/alert and status on the broker.

You can also navigate in the LCD by activating it by only using any button and then with the directional buttons you can deplace in the wanted view.

 
##

### How do i compile and flash ?

First you need to clone the git of ESP IDF anywere https://github.com/espressif/esp-idf.

You need also to have a declaration of IDF_PATH in your rc (like .bashrc), you can edit directly the file or add it temporatly with:

`export ESP_IDF=(the path to the git folder)`

After by using the Makefile, you just need to type `make` in the project folder.

To flash, you need to know the target its something like `/dev/ttyUSBX` to know just check the available one without connecting the ESP32 and check again when plugged.

Then type `make menuconfig`, go to `Serial Flasher Config` -> `Default Serial Port` then replace it by the one you've found(don't forget to save the config).

Finally just type `make flash` and then the project is upload in your ESP32.


##

### What do i need, to work properly ?

You need to have one MCU without microcontroller where we can find the sensors written below, and then configurate the BUS I2C in the `demo_conf.h`.
 
Then also 7 leds to show the differents state of the Wifi/Mqtt and Sensors, that you can configurate in the `demo_conf.h`.

Also one directional button, also to configure.

And finally a battery set, plugged on the ADC pin to know the actual battery percentage (current config for 3.7mAh).

**Reminder:** This project is based on ESP32 with Freertos with an LCD,Wifi and BLuetooth available.

##

### How do i create an update ?

You need to build your project with `make` and then get the `build/Board.bin` that you are going to rename `board-(version).bin`, then upload in the cloud where you stack your bins.

And to try it, you can modify the script `scripts/command/update_(version).sh` to the url of your mqtt to request an update.

##

### The scripts

You have a folder(`scripts`) with some scripts in it, in all the scripts you can modify the wanted url/channel, if you have changed it.

- To update to 1.0 (`scripts/command/update_1.0.sh`)
- To update to 1.1 (`scripts/command/update_1.1.sh`)
- To reboot (`scripts/command/reboot.sh`)
- To restart the wifi (`scripts/command/wifi_restart.sh`)
- To turn lcd on/off (`scripts/command/lcd_turn_on/off.sh`)
- To turn sensors on/off (`scripts/command/sensors_turn_on/off.sh`)
- To change the delay of refresh/emitting to 1000ms (`scripts/command/sensor_refresh_1000.sh`)
- To change the delay of refresh/emitting to 2000ms (`scripts/command/sensor_refresh_2000.sh`)

To see the flow of data emitted by the project, you can use `scripts/sub_to_datas.sh`

##

### The configuration

All the principal option that you can configure its in the `demo_conf.h`

- Version of the firmware `CURRENT_VERSION`(by default 1.1)
- Ssid of the targeted wifi router `DEFAULT_WIFI_SSID`(by default Honor Raphael)
- Password of the targeted wifi router `DEFAULT_WIFI_PASSWORD`(by default clemon69)
- Url to the targeted mqtt `DEFAULT_MQTT_URL`(by default tcp://iot.eclipse.org)
- Port to the targeted mqtt `DEFAULT_MQTT_PORT`(by default 1883)
- Name of the device by BLE `DEVICE_NAME`(by default Demonstrator)
- Buffer of the downloaded data `OTA_BUF_SIZE`(by default 1024)
- The url with variable to the bin file `BIN_URL`(by default http://.../board-%s.bin)
- The number of event stacked `STACK_QUEUE`(by default 10)
- The buffer used by the LCD, Wifi, BLE and Mqtt `BUFF_SIZE`(by default 512)

After you can find specific option about the configuration of thing.

##

### What sensors are we handling ?

 - Sensor Humidity/Temperature HTS221
 - Sensor Pressure/Temperature LPS22HB
 - Sensor Color RGBC BH1745NUCl

##
### Authors
 * [Raphael-G](https://github.com/Clemon-R)