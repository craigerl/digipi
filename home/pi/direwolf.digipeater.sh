#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
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

# create a custom direwolf conf file, based on detected audio and ptt method
cp /home/pi/direwolf.digipeater.conf /tmp/direwolf.digipeater.conf
USBPRESENT=`grep "USB" /proc/asound/cards | wc -l`
if [ $USBPRESENT -eq 0 ]; then
#  export ALSA_CARD=0
  sed -i 's/\#PTT GPIO/PTT GPIO/gi' /tmp/direwolf.digipeater.conf
else
#  export ALSA_CARD=1
  sed -i 's/\#PTT RIG/PTT RIG/gi' /tmp/direwolf.digipeater.conf
fi
sudo mv /tmp/direwolf.digipeater.conf /run/direwolf.digipeater.conf

direwolf -d t -p -q d -t 0 -c /run/direwolf.digipeater.conf | tee /home/pi/direwolf.log &

sudo /home/pi/direwatch.py --log "/run/direwolf.log" --title_text "APRS Digi"  &

# wait for direwolf to open port 8001
sleep 5

# bind bluetooth serial port to direwolf's KISS interface on port 8001
sudo rfcomm --raw watch /dev/rfcomm0 1 socat -d -d tcp4:127.0.0.1:8001 /dev/rfcomm0  > /tmp/rfcom.out 2>/tmp/rfcom.out

sleep infinity

exit 0
