#!/bin/bash
# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing direwolf in winlinke mode"
   sudo killall direwolf
   sudo killall kissattach
   sudo killall ax25d
   sudo killall direwatch.py
   exit 0
}



sudo systemctl stop digipeater
sudo killall kissattach
sudo ./direwolf.winlink.sh &   # direwolf must be root for kiss/ax25 to work?
sleep 5
sudo kissattach /tmp/kisstnc radio 44.56.4.222
sudo kissparms -c 1 -p radio  # fix invalid port first to tries on direwolf
sudo ax25d  # for rmsgw only

cd ~/linbpq
./pilinbpq &


while true;   
do
  /etc/rmsgw/updatechannel.py  # send our winlink status to winlink.org
  sleep 1200
done


sleep 10000000000
