#!/usr/bin/python3
import RPi.GPIO as GPIO
import subprocess
from socket import gethostname 
from socket import gethostbyname
from time import sleep

# GPIO buttons, pi tft uses logical 23 and 24 gpio pins
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(23,GPIO.IN)
GPIO.setup(24,GPIO.IN)

 
def button_callback_24(number):
    print("Button ",   number,  "  pressed.")
    if number == 24:                                       # start digipeater
        try:
            cmd = "sudo systemctl is-active digipeater"
            status = "active"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        except:
            status = "inactive"
        if status == "inactive":
            cmd = "sudo systemctl start digipeater"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        else:
            cmd = "hostname -I | cut -d' ' -f1"
            IP = subprocess.check_output(cmd, shell=True).decode("utf-8")
            if ( len(IP) < 5 ):
                IP = "0.0.0.0"
            cmd = "sudo systemctl stop digipeater"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
            cmd = "sudo /home/pi/digibanner.py -b Standby -s " + IP
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    return(0)


def button_callback_23(number):
    print("Button ",   number,  "  pressed.")
    if number == 23:                                        # start TNC/igate
        try:
            cmd = "sudo systemctl is-active tnc"
            status = "active"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        except:
            status = "inactive"
        if status == "inactive":
            cmd = "sudo systemctl start tnc"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        else:
            cmd = "hostname -I | cut -d' ' -f1"
            IP = subprocess.check_output(cmd, shell=True).decode("utf-8")
            if ( len(IP) < 5 ):
                IP = "0.0.0.0"
            cmd = "sudo systemctl stop tnc"
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
            cmd = "sudo /home/pi/digibanner.py -b Standby -s " + IP 
            cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    return(0)

GPIO.add_event_detect(24,GPIO.FALLING,callback=button_callback_24,bouncetime=2500)
GPIO.add_event_detect(23,GPIO.FALLING,callback=button_callback_23,bouncetime=2500)

# better way of doing nothing without an import?
while True:
   sleep(10000000)
