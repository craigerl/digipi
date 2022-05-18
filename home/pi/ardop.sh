#!/bin/bash
# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, killing ardop"
   killall piardopc
   exit 0
}

/home/pi/digibanner.py  -b "ARDOP" -s  "//digipi:8515 "

cd /tmp && /usr/local/bin/piardopc

