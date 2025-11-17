#!/usr/bin/python3

# direwatch

"""
Craig Lamparter KM6LYW,  2025, MIT Licnese

Code derived from Adafruit PIL python example

Usage:
  direwatch.py --log="/run/direwolf.log" --fontsize="30" --title_text="DigiPi"  -o
  (-o option displays one station at a time, full screen)

This will tail a direwolf log file and display callsigns on an
adafruit ST7789, ILI9341, ILI9486 tft display 

  https://www.adafruit.com/product/4484  
  https://www.adafruit.com/product/2423
  https://www.amazon.com/Resistive-Screen-IPS-Resolution-Controller/dp/B07V9WW96D

More information on the ST7789 display is here:

https://learn.adafruit.com/adafruit-mini-pitft-135x240-color-tft-add-on-for-raspberry-pi/python-setup

One of three display types should be specified on the command line
   st7789
   ili9341
   ili9486

SPI interface must be enabled in /boot/firmware/config.txt or in raspi-config

Do not install the kernel module/framebuffer.

GPIO pins 12 (PTT) and 16 (DCD) are monitored and light green/red icons respectively.
Configure these gpio pins in direwolf.

Much code taken from ladyada for her great work driving these devices,

Included ILI9486 python library taken from https://github.com/SirLefti/Python_ILI9486

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
import adafruit_rgb_display.ili9341 as ili9341 
#import ILI9486_gpiod as ili9486
import ILI9486 as ili9486
import pyinotify
import gpiod
from gpiod.line import Direction, Value
import threading
import signal
import os
import aprslib
import math
import numpy

# Config for display baudrate (default max is 24mhz):
BAUDRATE = 64000000

def parse_arguments():
    ap = argparse.ArgumentParser()
    ap.add_argument("-l", "--log", required=True, help="Direwolf log file location")
    ap.add_argument("-f", "--fontsize", required=False, help="Font size for callsigns")
    ap.add_argument("-t", "--title_text", required=False, help="Text displayed in title bar")
    ap.add_argument("-o", "--one", action='store_true', required=False, help="Show one station at a time full screen")
    ap.add_argument("-y", "--lat", required=False, help="Your Latitude -123.4567")
    ap.add_argument("-x", "--lon", required=False, help="Your Longitude  23.4567")
    ap.add_argument("-s", "--savefile", required=False, help="Save screen updates to png file")
    ap.add_argument("-d", "--display", required=False, help="st7789, ili9341, or ili9486")
    args = vars(ap.parse_args())
    return args

args = parse_arguments()
logfile = args["log"]
if args["fontsize"]:
   # 30 puts 6 lines
   # 33 puts 5 lines, max width
   fontsize = int(args["fontsize"])
else:
   fontsize = 30   # default 30
if args["title_text"]:
   title_text = args["title_text"]
else:
   title_text = "Direwatch"

if args["lat"]:
   lat1 = float(args["lat"])
else:
   lat1 = 0

if args["lon"]:
   lon1 = float(args["lon"])
else:
   lon1 = 0

if args["savefile"]:
   savefile = args["savefile"]
else:
   savefile = None

if args["display"]:
   displaytype = args["display"]
else:
   displaytype = st7789

if displaytype == 'ili9341':
   ## Large wide-screen adafruit screen 2.8" (320x240), two buttons
   spi = board.SPI()
   dc_pin = digitalio.DigitalInOut(board.D25)
   cs_pin = digitalio.DigitalInOut(board.D4)
   disp = ili9341.ILI9341(
       spi,
       cs=cs_pin,
       dc=dc_pin,
       baudrate=BAUDRATE,
       width=240,
       height=320,
       rotation=270
   )
elif displaytype == 'ili9486':
   ## Largest ili9486 display, no buttons
   from spidev import SpiDev
   spi = SpiDev(0,0)
   spi.mode = 0b10  
   spi.max_speed_hz = 48000000
   disp = ili9486.ILI9486(
       spi=spi,
       rst=25,
       dc=24,
       origin=ili9486.Origin.LOWER_RIGHT
   ).begin()
   disp.invert()
else:
   ## square adafruit screen 1.3" (240x240), two buttons
   dc_pin = digitalio.DigitalInOut(board.D25)
   cs_pin = digitalio.DigitalInOut(board.D4)
   spi = board.SPI()
   disp = st7789.ST7789(
       spi,
       cs=cs_pin,
       dc=dc_pin,
       baudrate=BAUDRATE,
       height=240,
       width=240,
       y_offset=80,
       rotation=180
   )

# don't write to display concurrently with thread
display_lock = threading.Lock()

# Create image and drawing object
if displaytype == 'st7789' or displaytype == 'ili9341':
   if disp.rotation % 180 == 90:
       height = disp.width    
       width = disp.height
   else:
       width = disp.width     
       height = disp.height
elif displaytype == 'ili9486': 
   width=480
   height=320
else:                         # sane default
   width=240
   height=240

#ili9486 must be in RGB format, no Alpha channel
#image = Image.new("RGBA", (width, height))
image = Image.new("RGB", (width, height))
draw = ImageDraw.Draw(image)

# define some constants to help with graphics layout
padding = 4 
title_bar_height = 34

chipdev = '/dev/gpiochip0'

#setup Blue LED
LINE = 5
blue_line = gpiod.request_lines( "/dev/gpiochip0", consumer="blue",
    config={
        LINE: gpiod.LineSettings(
            direction=Direction.OUTPUT, output_value=Value.INACTIVE
        )
    }
)

#setup Red LED
LINE = 26
red_line = gpiod.request_lines( "/dev/gpiochip0", consumer="blue",
    config={
        LINE: gpiod.LineSettings(
            direction=Direction.OUTPUT, output_value=Value.INACTIVE
        )
    }
)


def get_direction(origin, destination):
   lat1, lon1 = origin 
   lat2, lon2 = destination
   dLon = (lon2 - lon1)
   x = math.cos(math.radians(lat2)) * math.sin(math.radians(dLon))
   y = math.cos(math.radians(lat1)) * math.sin(math.radians(lat2)) - math.sin(math.radians(lat1)) * math.cos(math.radians(lat2)) * math.cos(math.radians(dLon))
   bearing = numpy.arctan2(x,y)
   bearing = numpy.degrees(bearing)
   bearing = (bearing + 360) % 360  # make positive
   dirs = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW']
   ix = int(round(bearing / (360. / len(dirs))))  
   return dirs[ix % len(dirs)]
   #return bearing

def get_distance(origin, destination):
   lat1, lon1 = origin
   lat2, lon2 = destination
   # radius = 6371  # km
   radius = 3959    # miles
   dlat = math.radians(lat2 - lat1)
   dlon = math.radians(lon2 - lon1)
   a = (math.sin(dlat / 2) * math.sin(dlat / 2) +
        math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) *
        math.sin(dlon / 2) * math.sin(dlon / 2))
   c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
   d = radius * c
   return d

def signal_handler(signal, frame):
   print("Got ", signal, " exiting.")
   draw.rectangle((0, 0, width, height), outline=0, fill=(30,30,30))
   with display_lock:
       disp.image(image)
   red_line.release()
   blue_line.release()
   os._exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# LED threads, bluetooth, RED, GREEN
def bluetooth_connection_poll_thread():  #FIXME convert to libgpiod, not gpiozero
    bt_status = 0
    time.sleep(2) # so screen initialization doesn't overdraw bluetooth as off

    while True:
        cmd = "hcitool con | wc -l"
        connection_count = subprocess.check_output(cmd, shell=True).decode("utf-8")
        if int(connection_count) > 1:
            if bt_status == 0:
                bt_status = 1
                bticon = Image.open('bt.small.on.png')   
                #blue_line.set_value(1) 
                blue_line.set_value(5, Value.ACTIVE)
                image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)
                with display_lock:
                    disp.image(image)
        else:
            if bt_status == 1:
                bt_status = 0  
                bticon = Image.open('bt.small.off.png')   
                #blue_line.set_value(0) 
                blue_line.set_value(5, Value.INACTIVE)
                image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)
                with display_lock:
                    disp.image(image)
        time.sleep(2)

bluetooth_thread = threading.Thread(target=bluetooth_connection_poll_thread, name="btwatch")
bluetooth_thread.start()

def redgreen_thread():                      ## change red or green status indicators and red diode
   f = subprocess.Popen(['tail','-F','-n','10',logfile], stdout=subprocess.PIPE,stderr=subprocess.PIPE)
   while True:
      line = f.stdout.readline().decode("utf-8", errors="ignore")
      search = re.match("^((?:DCD |PTT ).*)", line)
      if search is not None:
         status = search.group(1)
         if status == 'DCD 0 = 1':
            draw.ellipse(( width - title_bar_height     , padding,    width - padding * 2,                     title_bar_height - padding), fill=(0,230,0,255))
         elif status == 'DCD 0 = 0':
            draw.ellipse(( width - title_bar_height     , padding,    width - padding * 2,                     title_bar_height - padding), fill=(0,80,0,255))
         elif status == 'PTT 0 = 1':
            draw.ellipse(( width - title_bar_height * 2 , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(230,0,0,255))
            #red_led.on()
            #red_line.set_value(1)
            red_line.set_value(26, Value.ACTIVE)
         elif status == 'PTT 0 = 0':
            draw.ellipse(( width - title_bar_height * 2 , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,255))
            #red_led.off()
            #red_line.set_value(0)
            red_line.set_value(26, Value.INACTIVE)
         else:
            print("Unknown DCD/PTT event\n")
         with display_lock:
            disp.image(image)

redgreen_thread = threading.Thread(target=redgreen_thread, name="rgwatch")
redgreen_thread.start()


# Load a TTF font. 
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

if displaytype == 'ili9486':
   bump = 10
else:
   bump = 0

#font(no suffix) is defined on command line, used in list mode
font = ImageFont.truetype(fontpath, fontsize)
font_small = ImageFont.truetype(fontpath_bold, 18 + bump)
font_big = ImageFont.truetype(fontpath_bold, 24)            # title bar font
font_huge = ImageFont.truetype(fontpath_bold, 34 + bump)
font_epic = ImageFont.truetype(fontpath_bold, 38 + bump)

# load symbol chart based on font height
symbol_chart0x128 = Image.open("aprs-symbols-128-0.png")
symbol_chart1x128 = Image.open("aprs-symbols-128-1.png")
symbol_dimension = 128

# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=0, fill="#000000")

# Draw our logo
h = font.getbbox(title_text)[3]
draw.text(   (padding * 3  ,  height // 2 - h) ,   title_text, font=font_huge,   fill="#999999")
with display_lock:
    disp.image(image)
time.sleep(1)

# erase the screen
draw.rectangle((0, 0, width, height), outline=(0,0,0), fill="#000000")

# draw the header bar
draw.rectangle((0, 0, width, title_bar_height), fill=(30, 30, 30))
draw.text((padding, padding), title_text, font=font_big, fill="#999999")

# draw the bluetooth icon
bticon = Image.open('bt.small.off.png')
image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)

# draw Green LED
draw.ellipse(( width - title_bar_height               , padding,       width - padding * 2,                  title_bar_height - padding), fill=(0,80,0,255))

# draw Red LED
draw.ellipse(( width - title_bar_height * 2           , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,255))

with display_lock:
    disp.image(image)
    if savefile: image.save(savefile, compress_level=1) 

# fire up green/red led threads
#watch_threadG.start()
#watch_threadR.start()

# setup geometry defaults
call = "null"
x = padding
line_count = 0
col_count = 0 

# tail and block on the log file
f = subprocess.Popen(['tail','-F','-n','10',logfile], stdout=subprocess.PIPE,stderr=subprocess.PIPE)

# single loop, display one station at a time full screen, when using "-o" option on command line
def single_loop():

   # contstants
   infotopmargin = title_bar_height + ( padding * 4 )
   infolinespacing = font_small.getbbox("ABCJQ")[3] + padding

   # we try to get callsign, symbol and four relevant info lines from every packet
   while True:
      info1 = info2 = info3 = info4 = ''      # sane defaults

      line = f.stdout.readline().decode("utf-8", errors="ignore")
      
      search = re.search("^\[\d\.*\d*\] (.*)", line)

      if search is not None:
         packetstring = search.group(1)
         packetstring = packetstring.replace('<0x0d>','\x0d').replace('<0x1c>','\x1c').replace('<0x1e>','\x1e').replace('<0x1f>','\0x1f').replace('<0x0a>','\0x0a').replace('<0x20>','\0x20') 
      else:
         continue
  
      try:  
         packet = aprslib.parse(packetstring)                           # parse packet
         call = packet['from']
         supported_packet = True 
      except Exception as e:                                            # aprslib doesn't support all packet types
         supported_packet = False
         packet = {}   
         search = re.search("^\[\d\.*\d*\] ([a-zA-Z0-9-]*)", line)      # snag callsign from unsupported packet
         if search is not None:
            call = search.group(1) 
            symbol = '/'                                                # unsupported packet symbol set to bronze ball
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
            lat2 = packet['latitude']
            lon2 = packet['longitude']
            if lat1:
               distance = get_distance((lat1,lon1),(lat2,lon2))
               direction = get_direction((lat1,lon1),(lat2,lon2))   
               info1 = str(round(distance)) + "mi " + direction
            else:
               info1 = ""
            info2 = round(packet['weather']['temperature'])
            info2 = str(round(int(info2) * 1.8 + 32)) + 'F'
            info3 = ""
            info4 = str(packet['comment'])
         elif packet['format'] == 'mic-e' or packet['format'] == 'compressed'  or packet['format'] == 'uncompressed' or packet['format'] == 'object':  
            info4 = re.sub('^[^0-9a-zA-Z]*', '', packet['comment'])     # get rid of leading punctuation
            lat2 = packet['latitude']
            lon2 = packet['longitude']
            if lat1:
               distance = get_distance((lat1,lon1),(lat2,lon2))
               direction = get_direction((lat1,lon1),(lat2,lon2))
               info1 = str(round(distance)) + "mi " + direction
            else:
               info1 = ""
         elif 'status' in packet:                                       # status packet
            info4 = re.sub('^[^0-9a-zA-Z]+', '', packet['status'])      # get rid of leading punctuation
      except Exception as e:
         print("Malformed/missing data: ", str(e), ": ", packetstring)

      offset = ord(symbol) - 33
      row = offset // 16 
      col = offset % 16 
      draw.rectangle((0, title_bar_height, width, height), fill="#000000")  # erase most of screen
      crop_area = (col*symbol_dimension, row*symbol_dimension, col*symbol_dimension+symbol_dimension, row*symbol_dimension+symbol_dimension)
      if symbol_table == '/':
         symbolimage = symbol_chart0x128.crop(crop_area)
      else:
         symbolimage = symbol_chart1x128.crop(crop_area)
      #symbolimage = symbolimage.resize((height // 2, height // 2), Image.NEAREST)
      if height >= 320:
         symbolimage = symbolimage.resize((180, 180), Image.LANCZOS)
      image.paste(symbolimage, (0, title_bar_height), symbolimage)
      infoleftmargin = symbolimage.width + padding
      draw.text((infoleftmargin, infotopmargin),                         str(info1), font=font_small, fill="#AAAAAA")
      draw.text((infoleftmargin, infotopmargin + infolinespacing),       str(info2), font=font_small, fill="#AAAAAA")
      draw.text((infoleftmargin, infotopmargin + (infolinespacing * 2)), str(info3), font=font_small, fill="#AAAAAA")
      statustopmargin = symbolimage.height + title_bar_height 
      draw.text((5, statustopmargin), str(info4), font=font_small, fill="#AAAAAA")
      draw.text((5, height - font_epic.getbbox("J")[3]), call, font=font_epic, fill="#AAAAAA") # text up from bottom edge
  
      with display_lock:
          disp.image(image)
          if savefile: image.save(savefile, compress_level=1) 

      time.sleep(1)


# list of stations on screen (no -o option on command line)
def list_loop():
  call = "null"

  # scale symbols based on font height
  fontvertical = font.getbbox("ABCJQ")[3]       # tallest callsign, with dangling J/Q tails
  symbol_chart0x128.thumbnail(((fontvertical + fontvertical // 8) * 16, (fontvertical + fontvertical // 8) * 6)) # nudge larger than font, into space between lines
  symbol_chart1x128.thumbnail(((fontvertical + fontvertical // 8) * 16, (fontvertical + fontvertical // 8) * 6)) # nudge larger than font, into space between lines
  symbol_dimension = symbol_chart0x128.width//16
  max_line_width = font.getbbox("KN6MUC-15")[2] + symbol_dimension + (symbol_dimension // 8)   # longest callsign i can think of in pixels, plus symbo width + space
  max_cols = width // max_line_width

  # position cursor in -1 slot, as the first thing the loop does is increment slot
  y = padding + title_bar_height - font.getbbox("ABCJQ")[3]  
  x = padding
  line_height = font.getbbox("ABCJQ")[3] - 1          # tallest callsign, with dangling J/Q tails
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
          symbolimage = symbol_chart0x128.crop(crop_area)
       else:
          symbolimage = symbol_chart1x128.crop(crop_area)
       image.paste(symbolimage, (x, y), symbolimage)
       draw.text((x + symbol_dimension + (symbol_dimension // 8) , y), call, font=font, fill="#AAAAAA") # start text after symbol, relative padding
       line_count += 1
       with display_lock:
           disp.image(image)
           if savefile: image.save(savefile, compress_level=1) 

if __name__ == '__main__':
   if args["one"]:
      single_loop()
   else:
      list_loop()
   os._exit(0)
