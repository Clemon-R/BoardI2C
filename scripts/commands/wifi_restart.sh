#!/bin/bash

dpkg -s mosquitto-clients &> /dev/null
if [[ $? != 0 ]];
then
  sudo apt-get install mosquitto-clients
fi
mosquitto_pub -h demo1.rtower.io -d -t iot/dev/c4:4f:33:17:2b:b1/commands -m "{\"type\":3, \"restart\":true}"