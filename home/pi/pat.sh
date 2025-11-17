#!/bin/bash

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT
trap ctrl_c TERM
function ctrl_c() {
   echo "CTRL-C pressed, sending SIGINT to pat."
   sudo killall pat 
   exit 0
}

# see if we have ax25 or ardop running. if not, start ax25
if ! ( systemctl is-active ardop --quiet || systemctl is-active node --quiet || systemctl is-active winlinkrms --quiet ); then
   sudo systemctl start node
   sleep 2
fi


/usr/bin/pat http

