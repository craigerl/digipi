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

#cd /tmp && /usr/local/bin/piardopc.craig --leaderlength 200
#cd /tmp && /usr/local/bin/piardopc.john  --leaderlength 200 --trailerlength 200
#cd /tmp && /usr/local/bin/piardopc --leaderlength 200 --trailerlength 20
#cd /tmp && /usr/local/bin/piardopc  8515 ARDOP2IN ARDOP2OUT
#cd /tmp && /usr/local/bin/piardopc  8515 preARDOP2IN ARDOP2OUT
#cd /tmp && /usr/local/bin/piardopc  --leaderlength 200 

cd /tmp && /usr/local/bin/piardopc  
