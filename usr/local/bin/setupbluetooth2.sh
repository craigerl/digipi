#!/bin/sh

/usr/bin/sdptool add SP
bt-agent -d --capability=NoInputNoOutput
hciconfig hci0 piscan
