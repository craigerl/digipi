#!/bin/bash
# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing mercury"
   killall mercury
   sleep 10
   killall -9 mercury 2> /dev/null
   exit 0
}

source ~/localize.env

cp /home/pi/.config/mercury/mercury.ini.template /home/pi/.config/mercury/mercury.ini
sed -i "s/DEVICEFILE/$NEWDEVICEFILE/gi"  /home/pi/.config/mercury/mercury.ini
sed -i "s/RIGNUMBER/$NEWRIGNUMBER/gi"    /home/pi/.config/mercury/mercury.ini
sed -i "s/BAUDRATE/$NEWBAUDRATE/gi"      /home/pi/.config/mercury/mercury.ini

/home/pi/digibanner.py -b "MERCURY" -s "//digipi:8300 " -d $NEWDISPLAYTYPE

cd /home/pi/.config/mercury

/usr/local/bin/mercury
