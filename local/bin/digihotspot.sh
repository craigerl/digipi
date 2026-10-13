#!/bin/sh

echo "Auto-hotspot checking network status."

STATUS=`cat /sys/class/net/wlan0/operstate`

if [ $STATUS = "down" -o $STATUS = "dormant" ]; then
   echo "Wifi network is down, enabling hot spot mode."
   nmcli connection down wlan0
   sleep 1
   nmcli connection up hotspot
   sleep 1
   iptables -A INPUT -p tcp --dport 53 -j REJECT # php/lighttp hangs on DNS
fi

