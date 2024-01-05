cat /proc/device-tree/model  | grep "Raspberry Pi 5"
if [ $? -eq 1 ]; then
  echo "Pi5 detected"
  /home/pi/digibuttons.gpiod.py     #pi5
else
  echo "Pi5 not detected"
  /home/pi/digibuttons.rpigpio.py 
fi
