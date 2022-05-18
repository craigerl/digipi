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

# create a custom direwolf conf file, based on detected audio and ptt method
cp /home/pi/direwolf.winlink.conf /tmp/direwolf.winlink.conf
USBPRESENT=`grep "USB" /proc/asound/cards | wc -l`
if [ $USBPRESENT -eq 0 ]; then
  export ALSA_CARD=0
  sed -i 's/\#PTT GPIO/PTT GPIO/gi' /tmp/direwolf.winlink.conf
else
  export ALSA_CARD=1
  sed -i 's/\#PTT RIG/PTT RIG/gi' /tmp/direwolf.winlink.conf
fi
sudo mv /tmp/direwolf.winlink.conf /run/direwolf.winlink.conf

# set default alsa audio device
echo "ALSA_CARD $ALSA_CARD"

direwolf -d t -p -q d -t 0 -c /run/direwolf.winlink.conf | tee /home/pi/direwolf.log &

/home/pi/direwatch.py --log "/run/direwolf.log" --title_text "Winlink"  &

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



