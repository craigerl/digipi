#!/bin/sh

echo "Auto-hotspot checking network status."

STATUS=`cat /sys/class/net/wlan0/operstate`

if [ $STATUS = "down" -o $STATUS = "dormant" ]; then
   echo "Wifi network is down, enabling hot spot mode."
   nmcli connection up hotspot
fi

