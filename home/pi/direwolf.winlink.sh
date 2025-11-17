#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing direwolf in Winlink mode"
   sudo killall direwolf
   sudo killall rfcomm
   sudo killall -9 direwatch.py
   sudo killall ax25d
   sudo killall kissattach
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
cp /home/pi/direwolf.winlink.conf /tmp/direwolf.winlink.conf
USBPRESENT=`grep "USB" /proc/asound/cards | wc -l`
source <(head -n 25 localize.env)
if [ $NEWRIGNUMBER = DTR ]; then
  sed -i "s/\#PTT \/dev\/DEVICEFILE DTR/PTT \/dev\/$NEWDEVICEFILE DTR/gi" /tmp/direwolf.winlink.conf
elif [ $NEWRIGNUMBER = RTS ]; then
  sed -i "s/\#PTT \/dev\/DEVICEFILE RTS/PTT \/dev\/$NEWDEVICEFILE RTS/gi" /tmp/direwolf.winlink.conf
elif [ $NEWRIGNUMBER = CM108 ]; then
  sudo chown pi:audio /dev/$NEWDEVICEFILE
  sed -i "s/\#PTT CM108 DEVICEFILE/PTT CM108 \/dev\/$NEWDEVICEFILE/" /tmp/direwolf.winlink.conf
elif [ $USBPRESENT -eq 0 -o $NEWRIGNUMBER = GPIO ]; then
  sed -i "s/\#PTT GPIOD/PTT GPIOD/" /tmp/direwolf.winlink.conf
else
  sed -i "s/\#PTT RIG RIGNUMBER DEVICEFILE/PTT RIG $NEWRIGNUMBER \/dev\/$NEWDEVICEFILE/gi" /tmp/direwolf.winlink.conf
fi

sudo mv /tmp/direwolf.winlink.conf /run/direwolf.winlink.conf

direwolf -d t -d o -p -q d -t 0 -c /run/direwolf.winlink.conf |& grep --line-buffered -v PTT_METHOD > /home/pi/direwolf.log   &

/home/pi/direwatch.py --save "/run/direwatch.png" --log "/run/direwolf.log" --title_text "Winlink" --display $NEWDISPLAYTYPE  &

sleep 5 # wait for direwolf to create /tmp/kisstnc

sudo kissattach `ls -l /tmp/kisstnc | awk '{ print $11 }'` radio 44.56.4.222
sudo kissparms -c 1 -p radio  # fix invalid port first to tries on direwolf
sudo ax25d  # for rmsgw only
sudo route del -net 44.0.0.0 netmask 255.0.0.0  # kill tcp traffic to ax0, updatesysop.py hits api.winlink.org on net 44

# advertise our status to winlink backend
while true;
do
  /usr/local/bin/rmsgw_aci
  /etc/rmsgw/updatesysop.py
  sleep 1200
done


exit 0



