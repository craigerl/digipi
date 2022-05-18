#!/bin/sh
echo "Pairing mode.  Please look for \"digipi\" on your device."
#sudo remount
sudo systemctl restart bluetooth
sleep 5
sudo hciconfig hci0 piscan
sleep 2
sudo bluetoothctl discoverable on
sleep 2
sudo bluetoothctl scan on

echo "Stub: set trust in each device in /var/lib/bluetooth"

echo "Writing bluetooth information to SD Card."
echo "Please shutdown or reboot gracefully after this operation."

#sudo ./auto-agent.py
