#!/bin/bash

# trap ctrl-c and call ctrl_c()
#trap ctrl_c INT
#trap ctrl_c TERM
#function ctrl_c() {
#   touch /tmp/webchat_stop.txt
#   echo "CTRL-C pressed, sending SIGTERM to aprsd."
#   sudo killall aprsd 
#   sleep 2
#   sudo killall aprsd 
#   exit 0
#}

# see if we have a TNC running. if not, start 1200b tnc
if ! ( systemctl is-active tnc300b --quiet || systemctl is-active tnc --quiet || systemctl is-active tracker --quiet || systemctl is-active digipeater --quiet ); then 
   sudo systemctl start tnc
   sleep 2
fi

source ~/.aprsd-venv/bin/activate

# For longer loglines in journalctl -fu webchat output
export COLUMNS=200

# flush systemlog
#PYTHONUNBUFFERED=1

#aprsd webchat --port 8055 --loglevel DEBUG
aprsd webchat --port 8055 --loglevel ERROR

