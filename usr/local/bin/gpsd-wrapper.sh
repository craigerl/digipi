#!/bin/bash 

# gpsd on a piZero2 can't handle the icom705 gps, locks up.
# so we link ttyACM1 to a pipe and let gpsd read that
# 
# systemctl disable gpsd.socket
#
# /lib/systemd/system/gpsd.service :
# 
#[Unit]
#Description=GPS (Global Positioning System) Daemon
#After=chronyd.service
#[Service]
#EnvironmentFile=-/etc/default/gpsd
#ExecPreStart=/usr/bin/sleep 5
#ExecStart=/bin/bash -c /usr/local/bin/gpsd-wrapper.sh $GPSD_OPTIONS $OPTIONS $DEVICES
#
#[Install]
#WantedBy=multi-user.target

#BCM2835 devices lock up (zero2w, 3a+)

PIZERO=$(cat /proc/cpuinfo | grep "BCM2835" | wc -l)
ICOM705=$(lsusb | grep IC-705 | wc -l)

. /etc/default/gpsd

if [[ $PIZERO -ne 0  && $ICOM705 -ne 0 ]]; then
   mkfifo /tmp/gpsd.fifo
   /usr/sbin/gpsd $GPSD_OPTIONS $OPTIONS /tmp/gpsd.fifo
   sleep 1
   cat /dev/ttyACM1 > /tmp/gpsd.fifo &
else
   /usr/sbin/gpsd $GPSD_OPTIONS $OPTIONS $DEVICES
fi

sleep infinity
