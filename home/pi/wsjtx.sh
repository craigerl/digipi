#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing direwolf in winlinke mode"
   vncserver -kill :1
   sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
   sudo systemctl stop tnc
   sudo killall wsjtx
   sudo killall jt9
   sudo sh -c  "echo powersave  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

   exit 0
}

sudo sh -c  "echo performance  > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"

# stop stuff
vncserver -kill :1
sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
sudo systemctl stop tnc
sudo killall wsjtx
sudo killall jt9


# start stuff
nice -n 5 vncserver -depth 16                                 # runs in background
/home/pi/digibanner.py -b WSJTX\ FT8 -s http://digipi/ft8     # momentary run
#nice -n 5 /usr/share/novnc/utils/launch.sh &                  # this doesn't exit
nice -n 5 /usr/share/novnc/utils/novnc_proxy --vnc localhost:5901 &                # this doesn't exit

export DISPLAY=:1   

export LC_ALL=C; unset LANGUAGE  # wsjtx needs this

wsjtx &

sleep 8

#maximize app, full screen
wmctrl -a wsjt -b add,maximized_vert,maximized_horz

sleep 60

sudo renice -n 0 `ps aux | grep wsjtx | grep -v grep | awk '{print $2}'`
sudo renice -n 0 `ps aux | grep jt9 | grep -v grep | awk '{print $2}'`
sudo renice -n 5 `ps aux | grep Xtightvnc | grep -v grep | awk '{print $2}'`

sleep 6000000000

echo "URL is http://digipi:6080/vnc.html?host=digipi&port=6080&password=test11&autoconnect=true "



