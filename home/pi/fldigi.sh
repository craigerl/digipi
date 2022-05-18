#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing fldigi stuff."
   vncserver -kill :1
   sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
   sudo killall fldigi
   sudo sh -c  "echo powersave  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

   exit 0
}

sudo sh -c  "echo performance  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

# stop stuff
vncserver -kill :1
sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
sudo killall fldigi 
#sudo servivce wsjtx stop
#sudo servivce sstv stop

# start stuff
nice -n 5 vncserver -depth 16                               # runs in background
/home/pi/digibanner.py -b FLDigi -s http://digipi/fld       # exits quickly
nice -n 5 /usr/share/novnc/utils/launch.sh &                # this doesn't exit

export DISPLAY=:1   

#sudo swapon /dev/mmcblk0p2   # fldigi locks up without swap
fldigi &

sleep 8

#maximize app, full screen
wmctrl -a fldigi -b add,maximized_vert,maximized_horz

sudo renice -n 0 `ps aux | grep fldigi | grep -v grep | awk '{print $2}'`
sudo renice -n 5 `ps aux | grep Xtightvnc | grep -v grep | awk '{print $2}'`

sleep infinity

