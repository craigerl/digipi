#!/usr/bin/python3

# direwatch

"""
Craig Lamparter KM6LYW,  2021, MIT Licnese

This will tail a direwolf log file and display callsigns on an
adafruit st7789 tft display (https://www.adafruit.com/product/4484).  
Follow the instructions here to get the driver/library loaded:

https://learn.adafruit.com/adafruit-mini-pitft-135x240-color-tft-add-on-for-raspberry-pi/python-setup

Current configuration is for the 240x240 st7789 unit.

Do not install the kernel module/framebuffer.

GPIO pins 12 (PTT) and 16 (DCD) are monitored and light green/red icons respectively.
Configure these gpio pins in direwolf.


Installation on raspbian/buster for short-attentions span programmers like me:

sudo apt-get install python3-pip   # python >= 3.6 required
sudo pip3 install adafruit-circuitpython-rgb-display
sudo pip3 install pyinotify
sudo apt-get install python3-dev python3-rpi.gpio
vi /boot/config.txt  # uncomment following line: "dtparam=spi=on"
sudo pip3 install --upgrade adafruit-python-shell
wget https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/raspi-blinka.py
sudo python3 raspi-blinka.py   ## this gets the digitalio python module
sudo pip install aprslib     ## so we can parse ax.25 packets

Much code taken from ladyada for her great work driving these devices,
# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT
"""

import argparse
import digitalio
import board
from PIL import Image, ImageDraw, ImageFont
import adafruit_rgb_display.st7789 as st7789  
import os
import subprocess

# Configuration for CS and DC pins (these are PiTFT defaults):
cs_pin = digitalio.DigitalInOut(board.CE0)
dc_pin = digitalio.DigitalInOut(board.D25)
#reset_pin = digitalio.DigitalInOut(board.D24)

# Config for display baudrate (default max is 24mhz):
BAUDRATE = 64000000

# Setup SPI bus using hardware SPI:
spi = board.SPI()


# Use one and only one of these screen definitions:

## half height adafruit screen 1.1" (240x135), two buttons
#disp = st7789.ST7789(
#    board.SPI(),
#    cs=cs_pin,
#    dc=dc_pin,
#    baudrate=BAUDRATE,
#    width=135,
#    height=240,
#    x_offset=53,
#    y_offset=40,
#    rotation=270,
#)

# full height adafruit screen 1.3" (240x240), two buttons
disp = st7789.ST7789(
    spi,
    cs=cs_pin,
    dc=dc_pin,
    baudrate=BAUDRATE,
    height=240,
    y_offset=80,
    rotation=0
)

# don't write to display concurrently with thread
#display_lock = threading.Lock()

# Create image and drawing object
if disp.rotation % 180 == 90:
    height = disp.width  # we swap height/width to rotate it to landscape!
    width = disp.height
else:
    width = disp.width  # we swap height/width to rotate it to landscape!
    height = disp.height
image = Image.new("RGBA", (width, height))
draw = ImageDraw.Draw(image)

# define some constants to help with graphics layout
padding = 4 
title_bar_height = 34


#def signal_handler(signal, frame):
#   print("Got ", signal, " exiting.")
#   draw.rectangle((0, 0, width, height), outline=0, fill=(30,30,30))
#   with display_lock:
#       disp.image(image)
#   #sys.exit(0)  # thread ignores this
#   os._exit(0)

#signal.signal(signal.SIGINT, signal_handler)
#signal.signal(signal.SIGTERM, signal_handler)



def parse_arguments():
    ap = argparse.ArgumentParser()
#    ap.add_argument("-l", "--log", required=True, help="Direwolf log file location")
    ap.add_argument("-f", "--fontsize", required=False, help="Font size for callsigns")
    ap.add_argument("-b", "--big", required=False, help="large text to display")
    ap.add_argument("-s", "--small", required=False, help="smaller text underneath")
    ap.add_argument("-t", "--tiny", required=False, help="tiny text underneath smaller text")
    ap.add_argument("-i", "--ip", required=False, help="display IP address instead of tiny text")
    args = vars(ap.parse_args())
    return args

args = parse_arguments()

if args["fontsize"]:
   # 17 puts 11 lines 2 columns
   # 20 puts 9 lines
   # 25 puts 7 lines    
   # 30 puts 6 lines   ** default
   # 34 puts 5 lines, max width
   fontsize = int(args["fontsize"])
   if fontsize > 30:
      print("Look, this display isn't very wide, the maximum font size is 34pts, and you chose " + str(fontsize) + "?")
      print("Setting to 34 instead.")
      fontsize = 34
else:
   fontsize = 30

if args["big"]:
   big = args["big"]
else:
   big = "DigiPi"

if args["small"]:
   small = args["small"]
else:
   small = ""

if args["tiny"]:
   tiny = args["tiny"]
else:
   tiny = ""

if args["ip"]:
   cmd = "hostname -I | cut -d' ' -f1"
   IP = subprocess.check_output(cmd, shell=True).decode("utf-8")
   if ( len(IP) < 5 ):
       IP = "0.0.0.0"
   tiny = IP

font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", fontsize)
line_height = font.getsize("ABCJQ")[1] - 1          # tallest callsign, with dangling J/Q tails
font_huge = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 34)
font_big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)


# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=0, fill="#000000")

draw.rectangle((0, 0, width, 30), outline=0, fill="#333333")
draw.text(   (10  ,  0  ) , "DigiPi", font=font_big, fill="#888888")
draw.text(   (10  ,  height * .28 )                                   , big,   font=font_huge, fill="#888888")
draw.text(   (10  ,  height * .28 + font_big.getsize("BgJ")[1] + 12 ) , small, font=font_big,  fill="#666666")
draw.text(   (10  ,  height       - font_big.getsize("BgJ")[1] - 4  ) , tiny,  font=font_tiny, fill="#666666")
disp.image(image)

exit(0)

