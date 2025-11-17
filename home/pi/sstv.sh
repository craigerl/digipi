#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing qsstv stuff."
   vncserver -kill :1
   sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
   sudo killall qsstv
   sudo systemctl stop rigctld.service
   exit 0
}

sudo sh -c  "echo performance  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

source ~/localize.env

# stop stuff
vncserver -kill :1
sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
sudo killall qsstv 
sudo servivce wsjtx stop

# start stuff
vncserver -depth 16                                   # runs in background
/home/pi/digibanner.py -b SSTV -s http://digipi/tv  -d $NEWDISPLAYTYPE
/usr/share/novnc/utils/novnc_proxy --vnc localhost:5901 &

export DISPLAY=:1   

# qsstv fails to use hamlib
sudo systemctl start rigctld.service

qsstv &

sleep 1

#sudo renice -n 0 `ps aux | grep qsstv | grep -v grep | awk '{print $2}'`
#sudo renice -n 5 `ps aux | grep Xtightvnc | grep -v grep | awk '{print $2}'`

#maximize app, full screen
timeout=0
until  wmctrl -a qsstv -b add,maximized_vert,maximized_horz || [ $timeout -gt 20 ]
do
  sleep 1
  ((timeout+=1))
done
#sstv needs a second shot after spash screen success
sleep 4
wmctrl -a qsstv -b add,maximized_vert,maximized_horz 

sleep infinity

echo "URL is http://digipi:6080/vnc.html?host=digipi&port=6080&password=test11&autoconnect=true "



