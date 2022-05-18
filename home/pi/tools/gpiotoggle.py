#!/usr/bin/python
import RPi.GPIO as GPIO
import time
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(24,GPIO.OUT)

#while True:
print "On"
GPIO.output(24,GPIO.HIGH)
time.sleep(18)
print "Off"
GPIO.output(24,GPIO.LOW)
time.sleep(3)
