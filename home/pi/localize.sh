#!/bin/bash


Initialization values move to "localize.env", please edit that file.
  sudo remount
  sudo nano localize.env



# remount the root filesystem read/write
sudo remount

source /home/pi/localize.env

# set flag that we ran once, so we wont run again, for now
touch /var/cache/digipi/localized.txt

cd /

if [ -n "$NEWCALL" ]; then
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/uronode.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/uronode.perms
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/nrports 
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/axports
  sed -i "s/$OLDCALL/$NEWCALL/gi"           etc/ax25/ax25d.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.winlink.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.node.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/pat/config.json
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.digipeater.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.node.sh
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tnc.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/direwolf.tracker.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/channels.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/gateway.conf
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/banner
  sed -i "s/$OLDCALL/$NEWCALL/gi"           usr/local/etc/rmsgw/sysop.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/WSJT-X.ini
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/JS8Call.ini
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/fldigi/fldigi_def.xml
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/LinPac/macro/init.mac
  sed -i "s/$OLDCALL/$NEWCALL/gi"           home/pi/config/aprsd/aprsd.conf
fi

sed -i "s/COOL/DIGI/gi"                   etc/ax25/uronode.conf

if [ -n "$NEWNODEPASS" ]; then
  sed -i "s/$OLDNODEPASS/$NEWNODEPASS/gi"   etc/ax25/uronode.perms
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
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/direwolf.tracker.conf
  sed -i "s/$OLDAPRSPASS/$NEWAPRSPASS/gi"   home/pi/config/aprsd/aprsd.conf
fi


if [ -n "$NEWLAT" ]; then
  sed -i "s/$OLDLAT/$NEWLAT/gi"               home/pi/direwolf.tnc.conf
  sed -i "s/$OLDLAT/$NEWLAT/gi"               home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDLAT/$NEWLAT/gi"               home/pi/direwolf.digipeater.conf
  sed -i "s/$OLDLAT/$NEWLAT/gi"         home/pi/config/aprsd/aprsd.conf
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/config/aprsd/aprsd.conf

fi

if [ -n "$NEWLON" ]; then
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.tnc.conf
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.tnc300b.conf
  sed -i "s/$OLDLON/$NEWLON/gi"         home/pi/direwolf.digipeater.conf
fi

if [ -n "$NEWGPS" ]; then
  sed -i "1,/$OLDGPS/ s/$OLDGPS/$NEWGPS/" etc/default/gpsd  #first instance only
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
  if [ $NEWRIGNUMBER = "DTR" -o $NEWRIGNUMBER = "RTS" ]; then
    # hacky, wont run more than once
    sed -i "s/\-m $OLDRIGNUMBER \-r/\-P $NEWRIGNUMBER \-p/gi"  etc/systemd/system/rigctld.service
  elif [ $NEWRIGNUMBER = "GPIO" ]; then
    :  # do nothing
  else
    sed -i "s/$OLDRIGNUMBER/$NEWRIGNUMBER/gi"   etc/systemd/system/rigctld.service
  fi
fi

if [ -n "$NEWDEVICEFILE" ]; then
  sed -i "s/$OLDDEVICEFILE/$NEWDEVICEFILE/gi"   etc/systemd/system/rigctld.service
  for file in `ls /home/pi/direwolf*.conf`
  do
    sed -i "s/$OLDDEVICEFILE/$NEWDEVICEFILE/gi"   $file
  done
fi

if [ $NEWI2CAUDIO = "aiz" ] ||  [ $NEWI2CAUDIO = "drapizero" ]; then
  sed -i "s/\#dtoverlay=audioinjector-wm8731-audio/dtoverlay=audioinjector-wm8731-audio/gi"   boot/firmware/config.txt
  sed -i "s/dtoverlay=fe-pi-audio/\#dtoverlay=fe-pi-audio/gi"   boot/firmware/config.txt
fi

cd - > /dev/null 2> /dev/null

echo "DigiPi localizations made.  Please reboot or restart running services."


