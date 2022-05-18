#!/bin/bash

# Fill out these accordingly, using the examples below

NEWCALL=KX6XXX
NEWWLPASS=XXXXXX
NEWAPRSPASS=12345
NEWGRID=CN99mv
NEWLAT=40.9999N
NEWLON=140.9999W
NEWNODEPASS=abc123
NEWRIGNUMBER=3085
NEWDEVICEFILE=ttyACM0
NEWBAUDRATE=115200
NEWFLRIG=
NEWBIGVNC=







# Nothing to edit below here

OLDCALL=KX6XXX
OLDWLPASS=XXXXXX
OLDAPRSPASS=12345
OLDGRID=CN99mv
OLDLAT=40.9999N
OLDLON=140.9999W
OLDNODEPASS=abc123
OLDRIGNUMBER=3085
OLDDEVICEFILE=ttyACM0
OLDBAUDRATE=115200


# remount the root filesystem read/write
sudo remount

# set flag that we ran once, so we wont run again, for now
touch /var/cache/digipi/localized.txt

cd /


if [ -n "$NEWCALL" ]; then
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/nrports 
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/node.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/axports
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/node.perms
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/ax25d.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.winlink.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.node.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/pat/config.json
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.digipeater.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.node.sh
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/channels.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/gateway.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/banner
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/sysop.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/WSJT-X.ini
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/JS8Call.ini
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/fldigi/fldigi_def.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/LinPac/macro/init.mac
fi

sed -i "s/COOL/DIGI/gi"                   etc/ax25/node.conf

if [ -n "$NEWNODEPASS" ]; then
  sed -i "s/$OLDNODEPASS/$NEWNODEPASS/gi"   etc/ax25/node.perms
fi

if [ -n "$NEWWLPASS" ]; then
  sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       home/pi/config/pat/config.json
  sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       usr/local/etc/rmsgw/sysop.xml
fi

if [ -n "$NEWGRID" ]; then
  sed -i "s/$OLDGRID/$NEWGRID/gi"           home/pi/config/pat/config.json
  sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/sysop.xml
  sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/channels.xml
  sed -i "s/$OLDGRID/$NEWGRID/gi"           usr/local/etc/rmsgw/gateway.conf
  NEWGRIDCUT=`echo $NEWGRID | cut -c1-4`       # no grid suffix with ft8
  OLDGRID=`echo $OLDGRID | cut -c1-4`
  sed -i "s/$OLDGRID/$NEWGRIDCUT/gi"           home/pi/config/WSJT-X.ini
  sed -i "s/$OLDGRID/$NEWGRIDCUT/gi"           home/pi/config/JS8Call.ini
fi

if [ -n "$NEWAPRSPASS" ]; then
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.digipeater.conf
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.node.sh
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tnc.conf
fi


if [ -n "$NEWLAT" ]; then
  sed -i "s/$OLDLAT/$NEWLAT/gi"         home/pi/direwolf.tnc.conf
  sed -i "s/$OLDLAT/$NEWLAT/gi"         home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDLAT/$NEWLAT/gi"         home/pi/direwolf.digipeater.conf
fi

if [ -n "$NEWLON" ]; then
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.tnc.conf
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.digipeater.conf
fi

if [ -n "$NEWWLPASS" ]; then
  sed -i "s/$OLDWLPASS/$NEWWLPASS/gi"       usr/local/etc/rmsgw/channels.xml
fi

if [ -n "$NEWBAUDRATE" ]; then
  sed -i "s/$OLDBAUDRATE/$NEWBAUDRATE/gi"   etc/systemd/system/rigctld.service
fi

if [ -n "$NEWFLRIG" ]; then
  sed -i "s/\#flrig/flrig/gi"   home/pi/vnc/xstartup
fi

if [ -n "$NEWBIGVNC" ]; then
  sed -i "s/1024x768/1280x1024/gi"  etc/tightvncserver.conf
fi

if [ -n "$NEWRIGNUMBER" ]; then
  sed -i "s/$OLDRIGNUMBER/$NEWRIGNUMBER/gi"   etc/systemd/system/rigctld.service
  for file in `ls /home/pi/direwolf*.conf`
  do
    sed -i "s/RIG $OLDRIGNUMBER/RIG $NEWRIGNUMBER/gi"   $file
  done
fi

if [ -n "$NEWDEVICEFILE" ]; then
  sed -i "s/$OLDDEVICEFILE/$NEWDEVICEFILE/gi"   etc/systemd/system/rigctld.service
  for file in `ls /home/pi/direwolf*.conf`

  do
    sed -i "s/$OLDDEVICEFILE/$NEWDEVICEFILE/gi"   $file
  done
fi

cd - > /dev/null 2> /dev/null


echo "DigiPi localizations made.  Please reboot or restart running services."


