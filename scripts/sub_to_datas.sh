#!/bin/bash

dpkg -s mosquitto-clients &> /dev/null
if [[ $? != 0 ]];
then
  sudo apt-get install mosquitto-clients
fi
mosquitto_sub -h demo1.rtower.io -d -t iot/dev/c4:4f:33:17:2b:b1/data