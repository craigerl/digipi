#!/bin/sh

if pgrep -f "uvicorn" > /dev/null; then
  echo "uvicorn is already running, exiting."
  exit 0
fi 

sudo remount
sudo systemctl restart bluetooth 

sleep 2

. /home/pi/.bluetooth-venv/bin/activate
sudo /home/pi/.bluetooth-venv/bin/uvicorn bt_hub.main:app --host 0.0.0.0 --port 7684 --reload  
