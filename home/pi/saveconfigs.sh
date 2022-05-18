#!/bin/sh

echo "Writing runtime app configs to SD card."
sudo remount
cd ~
rsync -avH --delete /home/pi/.local/   /home/pi/local
rsync -avH --delete /home/pi/.config/  /home/pi/config
rsync -avH --delete /home/pi/.fldigi/  /home/pi/fldigi
rsync -avH --delete /home/pi/.flrig/   /home/pi/flrig


echo "Reboot to make filesystem read-only again."
