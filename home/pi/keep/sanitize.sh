#!/bin/bash

OLDWIFISSID=xxx
NEWWIFISSID=wifi_ssid

OLDWIFIPASS=cacadadafe
NEWWIFIPASS=abcdefghij

OLDCALL=KM6LYW
NEWCALL=KX6XXX

OLDWLPASS=7H7WWA
NEWWLPASS=XXXXXX

OLDAPRSPASS=22452
NEWAPRSPASS=12345

OLDGRID=CM98mv
NEWGRID=CN99mv

OLDLAT_A=3853.80N
NEWLAT_A=4099.99N

OLDLON_A=12056.15W
NEWLON_A=14099.99W

OLDLAT_B=38^55.40N
NEWLAT_B=40^99.99N

OLDLON_B=120^56.15W
NEWLON_B=140^99.99W

OLDNODEPASS=test11
NEWNODEPASS=abc123



sed -i "s/$OLDWIFIPASS/$NEWWIFIPASS/gi"   etc/wpa_supplicant/wpa_supplicant.conf 
sed -i "s/$OLDWIFISSID/$NEWWIFISSID/gi"   etc/wpa_supplicant/wpa_supplicant.conf 
sed -i "s/$OLDWIFIPASS/$NEWWIFIPASS/gi"   etc/hostapd/hostapd.conf 

sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/nrports 

sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/node.conf
sed -i "s/COOL/DIGI/gi"                   etc/ax25/node.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/axports

sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/node.perms
sed -i "s/$OLDNODEPASS/$NEWNODEPASS/gi"   etc/ax25/node.perms

sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/ax25d.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.winlink.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/wl2k/config.json
sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       home/pi/wl2k/config.json
sed -i "s/$OLDGRID/$NEWGRID/gi"           home/pi/wl2k/config.json

sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/sysop.xml
sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       usr/local/etc/rmsgw/sysop.xml
sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/sysop.xml

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.digipeater.conf
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.digipeater.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc300b.usb.conf
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tnc300b.usb.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/node.sh
sed -i "s/$OLDLAT_A/$NEWLAT_A/gi"         home/pi/node.sh
sed -i "s/$OLDLON_A/$NEWLON_A/gi"         home/pi/node.sh
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/node.sh

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc.conf
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tnc.conf
sed -i "s/$OLDLAT_B/$NEWLAT_B/gi"         home/pi/direwolf.tnc.conf
sed -i "s/$OLDLON_B/$NEWLON_B/gi"         home/pi/direwolf.tnc.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.digipeater.usb.conf
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.digipeater.usb.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc.usb.conf
sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tnc.usb.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.winlink.usb.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/channels.xml
sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       usr/local/etc/rmsgw/channels.xml
sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/channels.xml

sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/gateway.conf
sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/gateway.conf

sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/banner

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/WSJT-X.ini
NEWGRID=`echo $NEWGRID | cut -c1-4`
sed -i "s/$OLDGRID/$NEWGRID/gi"           home/pi/config/WSJT-X.ini           

sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/fldigi/fldigi_def.xml

rm var/cache/digipi/localized.txt
