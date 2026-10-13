#!/bin/bash -x

# Highly experimental Bluetooth TNC connection, see EDIT
# lines below.  Also edit direwolf.tnc.bt.conf with your
# callsign, aprs passcode.
# See DigiPi.org for pairing instructions prior to running
# this startup script.

trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing direwolf in TNC mode"
   sudo killall direwolf
   sudo killall rfcomm
   sudo killall -9 direwatch.py
   exit 0
}

# zero out old direwolf log file in case /run/ is full
truncate --size 0 /run/direwolf.log

# EDIT: add your bt device here
sudo rfcomm unbind /dev/rfcomm7 24:71:89:95:56:71 2  # Kenwood
sudo rfcomm unbind /dev/rfcomm7 38:D2:00:00:5D:CC 1  # Vero

# EDIT: add your bt device here
#bluetooth tnc connections 
sudo rfcomm bind /dev/rfcomm7 24:71:89:95:56:71 2  # Kenwood
#sudo rfcomm bind /dev/rfcomm7 38:D2:00:00:5D:CC 1  # Vero

#fake audio device
sudo modprobe snd-dummy

# prioritize USB audio device 
grep -i usb /proc/asound/cards > /dev/null 2>&1
if [ $? -eq 0 ]; then
   export ALSA_CARD=`grep -i usb /proc/asound/cards | head -1 | cut -c 2-2`
else
   export ALSA_CARD=0
fi
echo "ALSA_CARD:  $ALSA_CARD"

# create a custom direwolf conf file, based on detected ptt method
cp /home/pi/direwolf.tnc.bt.conf /tmp/direwolf.tnc.bt.conf
source <(head -n 25 localize.env)
USBPRESENT=`grep "USB" /proc/asound/cards | wc -l`
if [ $NEWRIGNUMBER = DTR ]; then  
  sed -i "s/\#PTT \/dev\/DEVICEFILE DTR/PTT \/dev\/$NEWDEVICEFILE DTR/gi" /tmp/direwolf.tnc.bt.conf 
elif [ $NEWRIGNUMBER = RTS ]; then  
  sed -i "s/\#PTT \/dev\/DEVICEFILE RTS/PTT \/dev\/$NEWDEVICEFILE RTS/gi" /tmp/direwolf.tnc.bt.conf
elif [ $NEWRIGNUMBER = CM108 ]; then  
  sudo chown pi:audio /dev/$NEWDEVICEFILE
  sed -i "s/\#PTT CM108 DEVICEFILE/PTT CM108 \/dev\/$NEWDEVICEFILE/" /tmp/direwolf.tnc.bt.conf
elif [ $USBPRESENT -eq 0 -o $NEWRIGNUMBER = GPIO ]; then
   sed -i "s/\#PTT GPIOD/PTT GPIOD/" /tmp/direwolf.tnc.bt.conf
else
  sed -i "s/\#PTT RIG RIGNUMBER DEVICEFILE/PTT RIG $NEWRIGNUMBER \/dev\/$NEWDEVICEFILE/gi" /tmp/direwolf.tnc.bt.conf
fi

sudo mv /tmp/direwolf.tnc.bt.conf /run/direwolf.tnc.bt.conf

direwolf-dev  -d h -d h -d h -d t -d o -p -q d -t 0 -c /run/direwolf.tnc.bt.conf |& grep --line-buffered -v PTT_METHOD > /home/pi/direwolf.log   &

sleep 1 # wait for direwolf to initialize gpio

# remove "-o" flag to see list of stations
if [ $NEWLAT = 39.9999 ]; then  # location not yet set
   /home/pi/direwatch.py -o --save "/run/direwatch.png" --log "/run/direwolf.log" --title_text "DigiPi TNC" --display $NEWDISPLAYTYPE  &
else
   /home/pi/direwatch.py -o --save "/run/direwatch.png" --log "/run/direwolf.log" --title_text "DigiPi TNC" --lat $NEWLAT --lon $NEWLON --display $NEWDISPLAYTYPE &
:

fi

# wait for direwolf to open port 8001
sleep 1

sleep infinity

exit 0
