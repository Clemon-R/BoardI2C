#!/bin/bash

dpkg -s mosquitto-clients &> /dev/null
if [[ $? != 0 ]];
then
  sudo apt-get install mosquitto-clients
fi
mosquitto_pub -h iot.eclipse.org -d -t /demo/rtone/esp32/commands -m "{\"type\":1}"