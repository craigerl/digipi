#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed."
   vncserver -kill :1
   sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
   sudo killall wsjtx
   sudo killall jt9
   exit 0
}


source ~/localize.env

# stop stuff
vncserver -kill :1
sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
#sudo systemctl stop tnc
sudo killall wsjtx
sudo killall jt9

# start stuff
nice -n 5 vncserver -depth 16                                 # runs in background
/home/pi/digibanner.py -b WSJTX\ FT8 -s http://digipi/ft8 -t http://digipi/logs -d $NEWDISPLAYTYPE    # momentary run
nice -n 5 /usr/share/novnc/utils/novnc_proxy --vnc localhost:5901 &   

export DISPLAY=:1   

export LC_ALL=C; unset LANGUAGE  # wsjtx needs this

wsjtx &

#maximize app, full screen
timeout=0
until  wmctrl -a wsjt -b add,maximized_vert,maximized_horz || [ $timeout -gt 20 ]
do
  sleep 1
  ((timeout+=1))
done

sleep 60

sudo renice -n 0 `ps aux | grep wsjtx | grep -v grep | awk '{print $2}'`
sudo renice -n 0 `ps aux | grep jt9 | grep -v grep | awk '{print $2}'`
sudo renice -n 5 `ps aux | grep Xtightvnc | grep -v grep | awk '{print $2}'`

echo "URL is http://digipi:6080/vnc.html?host=digipi&port=6080&password=test11&autoconnect=true "

sleep infinity

