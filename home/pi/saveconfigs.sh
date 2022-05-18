#!/bin/sh

echo "Writing runtime app configs to SD card."
sudo remount
cd ~
cp -a /home/pi/.local/*   /home/pi/local
cp -a /home/pi/.config/*  /home/pi/config
cp -a /home/pi/.fldigi/*  /home/pi/fldigi
cp -a /home/pi/.flrig/*  /home/pi/flrig


echo "Reboot to make filesystem read-only again."
