#!/usr/bin/python3
from gpiozero import Button
from signal import pause
import subprocess
from socket import gethostname 
from socket import gethostbyname

button24 = Button(24)
button23 = Button(23)

def button_callback_24():
    print("Button 24 button_callback24.\n")   #digipeater
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


def button_callback_23():
    print("Button 23 button_callback23.\n")   # TNC/igate
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

button23.when_pressed = button_callback_23
button24.when_pressed = button_callback_24


#sleep(100000000)

pause()

#try:
#    while True : pass
#except:
#    GPIO.cleanup()

