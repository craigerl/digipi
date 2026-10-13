#!/bin/bash -x

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing js8call stuff."
   vncserver -kill :1
   sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
   sudo killall js8call
   systemctl --user stop pipewire pipewire.socket pipewire-pulse pipewire-pulse.socket
   systemctl --user mask pipewire.socket

   exit 0
}

source ~/localize.env

# stop stuff
vncserver -kill :1
sudo kill `ps aux | grep launch | grep -v grep | awk '{print $2}'`  # novnc socket
sudo killall js8call 

# start stuff
nice -n 5 vncserver -depth 16                               # runs in background
/home/pi/digibanner.py -b JS8Call -s http://digipi/js8 -d $NEWDISPLAYTYPE
/usr/share/novnc/utils/novnc_proxy --vnc localhost:5901 &

export DISPLAY=:1
USER=$(id -u)
sudo systemctl start user@$USER.service  # populate /run/user/1000/
export XDG_RUNTIME_DIR=/run/user/$(id -u)
export DBUS_SESSION_BUS_ADDRESS="unix:path=${XDG_RUNTIME_DIR}/bus"
systemctl --user unmask pipewire.socket
systemctl --user start pipewire.socket pipewire pipewire-pulse

js8call &

#maximize app, full screen
timeout=0
until  wmctrl -a js8call -b add,maximized_vert,maximized_horz || [ $timeout -gt 20 ]
do
  sleep 1
  ((timeout+=1))
done

sudo renice -n 0 `ps aux | grep js8call | grep -v grep | awk '{print $2}'`
sudo renice -n 5 `ps aux | grep Xtightvnc | grep -v grep | awk '{print $2}'`

sleep infinity

systemctl --user stop pipewire pipewire.socket pipewire-pulse pipewire-pulse.socket
systemctl --user mask pipewire.socket

