#!/bin/sh

killall linpac 2> /dev/null

sudo rm /var/lock/LinPac* 2> /dev/null

axupped=0

axup=`ifconfig | grep ax0: | wc -l`

if [ $axup -eq 0 ]; then
   echo "No AX.25 device found."
   tnc1200up=`systemctl is-active tnc`
   tnc300up=`systemctl is-active tnc300b`
   if [ "$tnc1200up" = "active" ] || [ "$tnc300up" = "active" ]; then
      echo "TNC is active."
   else
      echo "TNC not active.  Starting 1200baud TNC."
      sudo systemctl start tnc
      sleep 4
   fi
   echo "Starting AX.25."
   sudo kissattach `ls -l /tmp/kisstnc | awk '{ print $11 }'` radio &
   sleep 2
   sudo kissparms -c 1 -p radio
   sudo modprobe netrom
   nrdevice=`ifconfig | grep nr0 | wc -l`
   if [ $nrdevice -eq 0 ]; then
     sudo nrattach netrom  # run this once per boot
   fi
   axupped=1
else
   echo "Using existing AX.25 interface."
fi

sleep 2

export LINES=40  # fix cropping on bottom of screen

linpac

# bring down ax.25 if we upped it here
if [ "$axupped" -eq "1" ]; then
   sudo killall kissattach
fi
