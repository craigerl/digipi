#!/usr/bin/python3

import pyinotify
import RPi.GPIO as GPIO
import os.path
from time import sleep

GPIO.setwarnings(False)

def handle_change(cb):
   with open('/sys/class/gpio/gpio12/value', 'r') as f:
      status = f.read(1)
      if status == '0':
         GPIO.output(26, GPIO.LOW)
      else:
         GPIO.output(26, GPIO.HIGH)
   f.close

def null_function(junk):  # default callback prints tons of debugging info
   return()

# wait for something to initialize PTT gpio pin
while not os.path.exists("/sys/class/gpio/gpio12/value"): 
   sleep(5)

print("Detected GPIO12 initialization.  Mirroring state to GPIO26.")

GPIO.setmode(GPIO.BCM)    # logical pin numbers, not BOARD
GPIO.setup(26, GPIO.OUT)  # Red LED
GPIO.setwarnings(False)   # suppress pin-is-in-use warning

# Instanciate a new WatchManager (will be used to store watches).
wm = pyinotify.WatchManager()

# Associate this WatchManager with a Notifier 
notifier = pyinotify.Notifier(wm, default_proc_fun=null_function)

# Add a new watch 
wm.add_watch('/sys/class/gpio/gpio12/value', pyinotify.IN_MODIFY)

# Loop forever and handle events.
notifier.loop(callback=handle_change)


