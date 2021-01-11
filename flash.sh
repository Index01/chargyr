#!/bin/bash

## todo: makefile actual

arduino-cli compile -b esp32:esp32:esp32 ./PowerMeUp.ino


arduino-cli upload -b esp32:esp32:esp32 -p /dev/ttyUSB0
