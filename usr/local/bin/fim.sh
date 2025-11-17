#!/bin/bash 


# disable getty1
#     sudo systemctl disable getty@tty1.service
# see supported video modes with kmsprint -m
# add this to /boot/firmware/cmdline.txt if you want 1280x720
#     consoleblank=0 loglevel=1 quiet logo.nologo video=1280x720
# add this to /boot/firmware/config.txt
#     dtoverlay=vc4-kms-v3d,audio=off

clear

echo ""
echo "Welcome to DigiPi."
echo ""
echo "Press Ctrl-Alt-F2 for Console."
echo ""

if [ ! -f /var/cache/digipi/localized.txt ]; then
   echo ""
   echo "This DigiPi has yet to be Initialized."
   echo ""
   echo "If you haven't already, please connect to the DigiPi wifi hotspot "
   echo "'DigiPi' (password=abcdefghij), then visit http://10.0.0.5/ for"
   echo "further configuration.  Additional information may be found at"
   echo "http://digipi.org."
   echo ""
   echo ""
   sleep 20 # wait for IP address
   echo "$(hostname -I | cut -f 1 -d\  ) "
   echo ""
   exit 0
fi

# bail if there's no monitor connected
if ! [ -c /dev/fb0 ]; then exit 0; fi

# wait for splash screen to appear, it sets the initial fbi resolution?
until [ -f /run/direwatch.png ]
do
     sleep 1
done

echo 0 > /sys/class/graphics/fbcon/cursor_blink

# wait for png to populate
sleep 2

sudo killall fim > /dev/null 2> /dev/null

#sudo chvt 1

ln -d /run/direwatch.png /run/direwatch1.png 
ln -d /run/direwatch.png /run/direwatch2.png 
sudo /usr/local/bin/fim -d /dev/fb0 -H -q /run/direwatch.png /run/direwatch1.png /run/direwatch2.png  > /tmp/fim.out 2> /tmp/fim.out

