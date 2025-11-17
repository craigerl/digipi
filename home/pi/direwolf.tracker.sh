#!/bin/bash -x

trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing direwolf in tracker mode"
   sudo killall direwolf
   sudo killall rfcomm
   sudo killall -9 direwatch.py
   exit 0
}

# zero out old direwolf log file in case /run/ is full
truncate --size 0 /run/direwolf.log

# prioritize USB audio device 
grep -i usb /proc/asound/cards > /dev/null 2>&1
if [ $? -eq 0 ]; then
   export ALSA_CARD=`grep -i usb /proc/asound/cards | head -1 | cut -c 2-2`
else
   export ALSA_CARD=0
fi
echo "ALSA_CARD:  $ALSA_CARD"

# create a custom direwolf conf file, based on detected ptt method
cp /home/pi/direwolf.tracker.conf /tmp/direwolf.tracker.conf
source <(head -n 25 localize.env)
USBPRESENT=`grep "USB" /proc/asound/cards | wc -l`
if [ $NEWRIGNUMBER = DTR ]; then  
  sed -i "s/\#PTT \/dev\/DEVICEFILE DTR/PTT \/dev\/$NEWDEVICEFILE DTR/gi" /tmp/direwolf.tracker.conf 
elif [ $NEWRIGNUMBER = RTS ]; then  
  sed -i "s/\#PTT \/dev\/DEVICEFILE RTS/PTT \/dev\/$NEWDEVICEFILE RTS/gi" /tmp/direwolf.tracker.conf
elif [ $NEWRIGNUMBER = CM108 ]; then
  sudo chown pi:audio /dev/$NEWDEVICEFILE
  sed -i "s/\#PTT CM108 DEVICEFILE/PTT CM108 \/dev\/$NEWDEVICEFILE/" /tmp/direwolf.tracker.conf
elif [ $USBPRESENT -eq 0 -o $NEWRIGNUMBER = GPIO ]; then
  sed -i "s/\#PTT GPIOD/PTT GPIOD/" /tmp/direwolf.tracker.conf
else
  sed -i "s/\#PTT RIG RIGNUMBER DEVICEFILE/PTT RIG $NEWRIGNUMBER \/dev\/$NEWDEVICEFILE/gi" /tmp/direwolf.tracker.conf
fi

sudo mv /tmp/direwolf.tracker.conf /run/direwolf.tracker.conf

direwolf -d t -d o -p -q d -t 0 -c /run/direwolf.tracker.conf |& grep --line-buffered -v PTT_METHOD > /home/pi/direwolf.log   &

sleep 1 # wait for direwolf to initialize gpio

#some day make direwatch dynamically update our location
/home/pi/direwatch.py -o --save "/run/direwatch.png" --log "/run/direwolf.log" --title_text "Tracker" --display $NEWDISPLAYTYPE  &
#/home/pi/direwatch.py -o --save "/run/direwatch.png" --log "/run/direwolf.log" --title_text "Tracker" --lat $NEWLAT --lon $NEWLON --display $NEWDISPLAYTYPE &

# wait for direwolf to open port 8001
sleep 1

# bind bluetooth serial port to direwolf's KISS interface on port 8001
sudo rfcomm --raw watch /dev/rfcomm0 1 socat -d -d tcp4:127.0.0.1:8001 /dev/rfcomm0  > /tmp/rfcom.out 2>/tmp/rfcom.out

sleep infinity

exit 0
