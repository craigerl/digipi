#!/usr/bin/python3
import select
import gpiod
import time
import threading
import subprocess
from signal import pause

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

def thread23():
   lastcalltime = 0
   chip = gpiod.Chip('/dev/gpiochip4')
   line23 = chip.get_line(23)
   line23.request(consumer="GPIN", type=gpiod.LINE_REQ_EV_FALLING_EDGE, flags=gpiod.LINE_REQ_FLAG_BIAS_PULL_UP)
   fd23 = line23.event_get_fd()
   poll = select.poll()
   poll.register(fd23)
   while True: 
      event = line23.event_read()
      if ((time.time() - lastcalltime) >= 1):  # one second debounce
         lastcalltime = time.time()
         button_callback_23(23)

def thread24():
   lastcalltime = 0 
   chip = gpiod.Chip('/dev/gpiochip4')
   line24 = chip.get_line(24)
   line24.request(consumer="GPIN", type=gpiod.LINE_REQ_EV_FALLING_EDGE, flags=gpiod.LINE_REQ_FLAG_BIAS_PULL_UP)
   fd24 = line24.event_get_fd()
   poll = select.poll()
   poll.register(fd24)
   while True:
      event = line24.event_read()
      if ((time.time() - lastcalltime) >= 1):  # one second debounce
         lastcalltime = time.time()
         button_callback_24(24)

t23 = threading.Thread(target=thread23)
t23.start()

t24 = threading.Thread(target=thread24)
t24.start()

pause()

