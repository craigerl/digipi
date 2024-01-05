#!/usr/bin/python3

# direwatch

"""
Craig Lamparter KM6LYW,  2022, MIT Licnese

Code derived from Adafruit PIL python example

This will tail a direwolf log file and display callsigns on an
adafruit st7789 tft display (https://www.adafruit.com/product/4484).  
Follow the instructions here to get the driver/library loaded:

https://learn.adafruit.com/adafruit-mini-pitft-135x240-color-tft-add-on-for-raspberry-pi/python-setup

Current configuration is for the 240x240 st7789 unit.

Do not install the kernel module/framebuffer.

GPIO pins 12 (PTT) and 16 (DCD) are monitored and light green/red icons respectively.
Configure these gpio pins in direwolf.

Installation on raspbian/bullseye for short-attentions span programmers like me:

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
import time
import subprocess
import digitalio
import board
from PIL import Image, ImageDraw, ImageFont
import re
import adafruit_rgb_display.st7789 as st7789  
import pyinotify
#import RPi.GPIO as GPIO
from gpiozero import LED
import threading
import signal
import os
import aprslib

# Configuration for CS and DC pins (these are PiTFT defaults):
#cs_pin = digitalio.DigitalInOut(board.CE0)
#dc_pin = digitalio.DigitalInOut(board.D25)
dc_pin = digitalio.DigitalInOut(board.D25)
cs_pin = digitalio.DigitalInOut(board.D4)

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
    rotation=180
)

# don't write to display concurrently with thread
display_lock = threading.Lock()

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

def signal_handler(signal, frame):
   print("Got ", signal, " exiting.")
   draw.rectangle((0, 0, width, height), outline=0, fill=(30,30,30))
   with display_lock:
       disp.image(image)
   #sys.exit(0)  # thread ignores this
   os._exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


def parse_arguments():
    ap = argparse.ArgumentParser()
    ap.add_argument("-l", "--log", required=True, help="Direwolf log file location")
    ap.add_argument("-f", "--fontsize", required=False, help="Font size for callsigns")
    ap.add_argument("-t", "--title_text", required=False, help="Text displayed in title bar")
    ap.add_argument("-o", "--one", action='store_true', required=False, help="Show one station at a time full screen")
    args = vars(ap.parse_args())
    return args

args = parse_arguments()
logfile = args["log"]
if args["fontsize"]:
   # 30 puts 6 lines
   # 33 puts 5 lines, max width
   fontsize = int(args["fontsize"])
   if fontsize > 33:
      print("Look, this display isn't very wide, the maximum font size is 33pts, and you chose " + str(fontsize) + "?")
      print("Setting to 33 instead.")
      fontsize = 33 
else:
   fontsize = 30   # default 30
if args["title_text"]:
   title_text = args["title_text"]
else:
   title_text = "Direwatch"


# LED threads, bluetooth, RED, GREE"N

def bluetooth_connection_poll_thread():
    bt_status = 0
    #GPIO.setmode(GPIO.BCM)
    #GPIO.setup(5, GPIO.OUT) 
    blue_led = LED(5)
    time.sleep(2) # so screen initialization doesn't overdraw bluetooth as off

    while True:
        cmd = "hcitool con | wc -l"
        connection_count = subprocess.check_output(cmd, shell=True).decode("utf-8")
        if int(connection_count) > 1:
            if bt_status == 0:
                bt_status = 1
                bticon = Image.open('bt.small.on.png')   
                #GPIO.output(5, GPIO.HIGH)
                blue_led.on() 
                image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)
                with display_lock:
                    disp.image(image)
        else:
            if bt_status == 1:
                bt_status = 0  
                bticon = Image.open('bt.small.off.png')   
                #GPIO.output(5, GPIO.LOW)
                blue_led.off()
                image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)
                with display_lock:
                    disp.image(image)
        time.sleep(2)

bluetooth_thread = threading.Thread(target=bluetooth_connection_poll_thread, name="btwatch")
bluetooth_thread.start()


def red_led_from_logfile_thread():                               ## RED logfile
   #print("red led changing via logfile")
   #f = subprocess.Popen(['tail','-F',logfile], stdout=subprocess.PIPE,stderr=subprocess.PIPE)
   f = subprocess.Popen(['tail','-F',logfile], stdout=subprocess.PIPE,stderr=subprocess.PIPE)
   while True:
      line = f.stdout.readline().decode("utf-8", errors="ignore")
      search = re.search("^\[\d[A-Z]\]", line)
      if search is not None:
         draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(200,0,0,0))
         with display_lock:
            disp.image(image)
         time.sleep(1)
         draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,0))
         with display_lock:
            disp.image(image)


def handle_changeG(cb):
   with open('/sys/class/gpio/gpio16/value', 'r') as f:          ## GREEN
      status = f.read(1)
      if status == '0':
         draw.ellipse(( width - title_bar_height               , padding,       width - padding * 2,                  title_bar_height - padding), fill=(0,80,0,0))
      else:
         draw.ellipse(( width - title_bar_height               , padding,       width - padding * 2,                  title_bar_height - padding), fill=(0,200,0,0))
      with display_lock:
         disp.image(image)
   f.close

def handle_changeR(cb):
   #print("red led changing via gpio")
   with open('/sys/class/gpio/gpio12/value', 'r') as f:          ## RED GPIO
      status = f.read(1)
      if status == '0':
         draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,0))
      else:
         draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(200,0,0,0))
         pass
      with display_lock:
         disp.image(image)
   f.close

def null_function(junk):  # default callback prints tons of debugging info
   return()

# Instanciate a new WatchManager (will be used to store watches).
wmG = pyinotify.WatchManager()
wmR = pyinotify.WatchManager()

# Associate this WatchManager with a Notifier
notifierG = pyinotify.Notifier(wmG, default_proc_fun=null_function)
notifierR = pyinotify.Notifier(wmR, default_proc_fun=null_function)

# Watch both gpio pins for change if they exist
wmG.add_watch('/sys/class/gpio/gpio16/value', pyinotify.IN_MODIFY)
if os.path.exists("/sys/class/gpio/gpio12/value"):
   wmR.add_watch('/sys/class/gpio/gpio12/value', pyinotify.IN_MODIFY)

watch_threadG = threading.Thread(target=notifierG.loop, name="led-watcherG", kwargs=dict(callback=handle_changeG))

# Use gpio pin for red led if it exists, otherwise watch log file for transmit activity
if os.path.exists("/sys/class/gpio/gpio12/value"):
   watch_threadR = threading.Thread(target=notifierR.loop, name="led-watcherR", kwargs=dict(callback=handle_changeR))
else:
   watch_threadR = threading.Thread(target=red_led_from_logfile_thread, name="redledthreadlog")


# Load a TTF font.  Make sure the .ttf font file is in the
# same directory as the python script!
# Some other nice fonts to try: http://www.dafont.com/bitmap.php
fontname = "DejaVuSans.ttf"
fontname_bold = "DejaVuSans-Bold.ttf"
if os.path.exists("/usr/share/fonts/truetype/dejavu/" + fontname):
   fontpath = "/usr/share/fonts/truetype/dejavu/" + fontname
elif os.path.exists("./" + fontname):
   fontpath = "./" + fontname
else:
   print("Couldn't find font " +  fontname + " in working dir or /usr/share/fonts/truetype/dejavu/")
   exit(1)
if os.path.exists("/usr/share/fonts/truetype/dejavu/" + fontname_bold):
   fontpath_bold = "/usr/share/fonts/truetype/dejavu/" + fontname_bold
elif os.path.exists("./" + fontname_bold):
   fontpath_bold = "./" + fontname_bold
else:
   print("Couldn't find font " +  fontname_bold + " in working dir or /usr/share/fonts/truetype/dejavu/")
   exit(1)
font = ImageFont.truetype(fontpath, fontsize)
font_small = ImageFont.truetype(fontpath_bold, 18)
font_big = ImageFont.truetype(fontpath_bold, 24)
font_huge = ImageFont.truetype(fontpath_bold, 34)
font_epic = ImageFont.truetype(fontpath, 40)
#font = ImageFont.truetype("/usr/share/fonts/truetype/dafont/BebasNeue-Regular.ttf", fontsize)
#font_big = ImageFont.truetype("/usr/share/fonts/truetype/dafont/BebasNeue-Regular.ttf", 24)
#font_huge = ImageFont.truetype("/usr/share/fonts/truetype/dafont/BebasNeue-Regular.ttf", 34)
#line_height = font.getsize("ABCJQ")[1] - 1          # tallest callsign, with dangling J/Q tails
line_height = font.getbbox("ABCJQ")[3] - 1          # tallest callsign, with dangling J/Q tails


# load and scale symbol chart based on font height
symbol_chart0x64 = Image.open("aprs-symbols-64-0.png")
symbol_chart1x64 = Image.open("aprs-symbols-64-1.png")
#fontvertical = font.getsize("XXX")[1]
fontvertical = font.getbbox("ABCJQ")[3]       # tallest callsign, with dangling J/Q tails
symbol_chart0x64.thumbnail(((fontvertical + fontvertical // 8) * 16, (fontvertical + fontvertical // 8) * 6)) # nudge larger than font, into space between lines
symbol_chart1x64.thumbnail(((fontvertical + fontvertical // 8) * 16, (fontvertical + fontvertical // 8) * 6)) # nudge larger than font, into space between lines
symbol_dimension = symbol_chart0x64.width//16

#max_line_width = font.getsize("KN6MUC-15")[0] + symbol_dimension + (symbol_dimension // 8)   # longest callsign i can think of in pixels, plus symbo width + space
max_line_width = font.getbbox("KN6MUC-15")[2] + symbol_dimension + (symbol_dimension // 8)   # longest callsign i can think of in pixels, plus symbo width + space
max_cols = width // max_line_width

# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=0, fill="#000000")

# Draw our logo
#w,h = font.getsize(title_text)
h = font.getbbox(title_text)[3]
draw.text(   (padding * 3  ,  height // 2 - h) ,   title_text, font=font_huge,   fill="#99AA99")
with display_lock:
    disp.image(image)
time.sleep(1)

# erase the screen
draw.rectangle((0, 0, width, height), outline=0, fill="#000000")

# draw the header bar
draw.rectangle((0, 0, width, title_bar_height), fill=(30, 30, 30))
draw.text((padding, padding), title_text, font=font_big, fill="#99AA99")

# draw the bluetooth icon
bticon = Image.open('bt.small.off.png')
image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)

# draw Green LED
draw.ellipse(( width - title_bar_height               , padding,       width - padding * 2,                  title_bar_height - padding), fill=(0,80,0,0))

# draw Red LED
draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,0))

with display_lock:
    disp.image(image)

# fire up green/red led threads
watch_threadG.start()
watch_threadR.start()

# setup screen geometries
call = "null"
x = padding
max_lines  = ( height - title_bar_height - padding )  //   line_height 
max_cols = ( width // max_line_width )
line_count = 0
col_count = 0 

# tail and block on the log file
f = subprocess.Popen(['tail','-F','-n','10',logfile], stdout=subprocess.PIPE,stderr=subprocess.PIPE)

# Display loops.  list of stations, or a single station on the screen at a time

def single_loop():
   symbol_chart0x64 = Image.open("aprs-symbols-64-0.png")
   symbol_chart1x64 = Image.open("aprs-symbols-64-1.png")

   # we try to get callsign, symbol and four relevant info lines from every packet
   while True:
      info1 = info2 = info3 = info4 = ''      # sane defaults

      line = f.stdout.readline().decode("utf-8", errors="ignore")
      
      search = re.search("^\[\d\.*\d*\] (.*)", line)

      if search is not None:
         packetstring = search.group(1)
         packetstring = packetstring.replace('<0x0d>','\x0d').replace('<0x1c>','\x1c').replace('<0x1e>','\x1e').replace('<0x1f>','\0x1f').replace('<0x0a>','\0x0a')
      else:
         continue
  
      try:  
         packet = aprslib.parse(packetstring)                           # parse packet
         #print(packet)
         call = packet['from']
         supported_packet = True 
      except Exception as e:                                            # aprslib doesn't support all packet types
         #print("Exception: aprslib: ", str(e), ": ", packetstring)
         supported_packet = False
         packet = {}   
         search = re.search("^\[\d\.*\d*\] ([a-zA-Z0-9-]*)", line)        # snag callsign from unsupported packet
         if search is not None:
            call = search.group(1) 
            symbol = '/'                                                # unsupported packet symbol set to red ball
            symbol_table = '/'
         else:
            continue

      try:          
         if supported_packet:
            if 'symbol' in packet:                                      # get symbol from valid packet or use red ball
               symbol = packet['symbol']
               symbol_table = packet['symbol_table']
            else:
               symbol = '/'
               symbol_table = '/'
                                                                        # extract relevant info lines
         if not supported_packet:                                      
               info1 = info2 = info3 = info4 = ''                       # no info in unsupported packet
         elif 'weather' in packet:                                      # weather (often contained in compressed/uncompressed type packets)
            info1 = round(packet['weather']['temperature'])
            info1 = str(int(info1) * 1.8 + 32) + 'F'
            #print(info1)
            info2 = str(packet['weather']['rain_since_midnight']) + '\" rain'
            #print(info2)
            info3 = str(round(packet['weather']['wind_speed'])) + ' m/h'
            info3 = info3 + ' ' + str(packet['weather']['wind_direction']) + '\''
            #print(info3)
            info4 = str(packet['comment'])
            #print(info4)                                                # position packet
         elif packet['format'] == 'mic-e' or packet['format'] == 'compressed'  or packet['format'] == 'uncompressed' or packet['format'] == 'object':  
            info4 = packet['comment']                                   # fixme: comment is jibberish in all compressed packets                           
         elif 'status' in packet:                                       # status packet
            info4 = packet['status']
      except Exception as e:
         print("Malformed/missing data: ", str(e), ": ", packetstring)

      symbol_dimension = 64
      offset = ord(symbol) - 33
      row = offset // 16 
      col = offset % 16 
      y = height // 3 
      x = width // 3  
      draw.rectangle((0, title_bar_height, width, height), outline=0, fill="#000000")  # erase most of screen
      crop_area = (col*symbol_dimension, row*symbol_dimension, col*symbol_dimension+symbol_dimension, row*symbol_dimension+symbol_dimension)
      if symbol_table == '/':
         symbolimage = symbol_chart0x64.crop(crop_area)
      else:
         symbolimage = symbol_chart1x64.crop(crop_area)
      symbolimage = symbolimage.resize((height // 2, height // 2), Image.NEAREST)
      #image.paste(symbolimage, (0, 36), symbolimage)
      image.paste(symbolimage, (0, title_bar_height), symbolimage)
      draw.text((120, 50), str(info1), font=font_small, fill="#AAAAAA")
      draw.text((120, 70), str(info2), font=font_small, fill="#AAAAAA")
      draw.text((120, 90), str(info3), font=font_small, fill="#AAAAAA")
      draw.text((5, 144), str(info4), font=font_small, fill="#AAAAAA")
      #draw.text((5, height - font_epic.getsize("X")[1] - 3), call, font=font_epic, fill="#AAAAAA") # text up from bottom edge
      draw.text((5, height - font_epic.getbbox("X")[3] - 3), call, font=font_epic, fill="#AAAAAA") # text up from bottom edge
  
      with display_lock:
          disp.image(image)
      time.sleep(1)


# Display loops.  list of stations, or a single station on the screen at a time
def list_loop():
  call = "null"
  # position cursor in -1 slot, as the first thing the loop does is increment slot
  #y = padding + title_bar_height - font.getsize("ABCJQ")[1]  
  y = padding + title_bar_height - font.getbbox("ABCJQ")[3]  
  x = padding
  max_lines  = ( height - title_bar_height - padding )  //   line_height 
  max_cols = ( width // max_line_width )
  line_count = 0
  col_count = 0 

  while True:
    line = f.stdout.readline().decode("utf-8", errors="ignore")

    # watch for regular packet
    #search = re.search("^\[\d\.\d\] (.*)", line)
    #Direwolf lines start with [#.#], or just [#], observed on HF
    search = re.search("^\[\d\.*\d*\] (.*)", line)

    if search is not None:
       packetstring = search.group(1)
       packetstring = packetstring.replace('<0x0d>','\x0d').replace('<0x1c>','\x1c').replace('<0x1e>','\x1e').replace('<0x1f>','\0x1f').replace('<0x0a>','\0x0a')
    else:
       continue

    lastcall = call

    try:                                        #   aprslib has trouble parsing all packets
       packet = aprslib.parse(packetstring) 
       call = packet['from']
       if 'symbol' in packet:
          symbol = packet['symbol']
          symbol_table = packet['symbol_table']
       else:
          symbol = '/'
          symbol_table = '/'
    except:                                     #   if it fails, let's just snag the callsign
       #print("aprslib failed to parse.")
       #search = re.search("^\[\d\.\d\] ([a-zA-Z0-9-]*)", line)
       search = re.search("^\[\d\.*\d*\] ([a-zA-Z0-9-]*)", line)
       if search is not None:
          call = search.group(1) 
          symbol = '/'
          symbol_table = '/'
       else:
          continue

    offset = ord(symbol) - 33
    row = offset // 16 
    col = offset % 16 

    if call == lastcall:   # blink duplicates
       time.sleep(0.5)
       draw.text((x + symbol_dimension + (symbol_dimension // 8) , y), call, font=font, fill="#000000") # start text after symbol, relative padding
       with display_lock:
           disp.image(image)
       time.sleep(0.1)
       draw.text((x + symbol_dimension + (symbol_dimension // 8) , y), call, font=font, fill="#AAAAAA") # start text after symbol, relative padding
       with display_lock:
           disp.image(image)
    else:
       y += line_height
       if line_count == max_lines:       # about to write off bottom edge of screen
           col_count += 1
           x = col_count * max_line_width  
           y = padding + title_bar_height
           line_count = 0
       if col_count == max_cols:         # about to write off right edge of screen
           x = padding 
           y = padding + title_bar_height
           draw.rectangle((0, title_bar_height + 1, width, height), outline=0, fill="#000000") # erase lines
           line_count = 0
           col_count = 0
           time.sleep(2.0)
       crop_area = (col*symbol_dimension, row*symbol_dimension, col*symbol_dimension+symbol_dimension, row*symbol_dimension+symbol_dimension)
       if symbol_table == '/':
          symbolimage = symbol_chart0x64.crop(crop_area)
       else:
          symbolimage = symbol_chart1x64.crop(crop_area)
       image.paste(symbolimage, (x, y), symbolimage)
       draw.text((x + symbol_dimension + (symbol_dimension // 8) , y), call, font=font, fill="#AAAAAA") # start text after symbol, relative padding
       line_count += 1
       with display_lock:
           disp.image(image)

if args["one"]:
   single_loop()
else:
   list_loop()


exit(0)

