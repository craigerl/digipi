#!/bin/bash

# indicate digipi is online with network status


# abort if direwatch is already running

RUNNING=`pgrep direwatch.py | wc -l`
if [ $RUNNING -ne 0 ]; then
   exit 0
fi

. /home/pi/localize.env

IP=$(hostname -I | cut -f 1 -d\  )

/home/pi/digibanner.py -b DigiPi -s "Online" -t "$IP" -g /home/pi/km6lyw.png -d $NEWDISPLAYTYPE

sleep 2

IP=$(hostname -I | cut -f 1 -d\  )
if [ $IP == "10.0.0.5" ]; then
  /home/pi/digibanner.py -b DigiPi -s "Hotspot" -t "$IP" -g /home/pi/km6lyw.png -d $NEWDISPLAYTYPE
else
  /home/pi/digibanner.py -b DigiPi -s "http://digipi" -t "$IP" -g /home/pi/km6lyw.png -d $NEWDISPLAYTYPE
fi


