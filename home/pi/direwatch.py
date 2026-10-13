#!/usr/bin/python3 -W ignore::SyntaxWarning

# direwatch

"""
Craig Lamparter KM6LYW,  2026, MIT Licnese

Code derived from Adafruit PIL python example

This will tail a direwolf log file and display callsigns on 
HDMI, DSI, ST7789, ILI9341, ILI9486, mhs35 tft display 

One of these display types should be specified on the command line
   st7789
   ili9341
   ili9486
   mhs35
   hdmi 
   dsi

SPI interface must be enabled in /boot/firmware/config.txt or in raspi-config

Do not install the kernel module/framebuffer.

GPIO pins 12 (PTT) and 16 (DCD) are monitored and light green/red icons respectively.
Configure these gpio pins in direwolf.

Much code taken from ladyada for her great work driving these devices,

Included ILI9486 python library taken from https://github.com/SirLefti/Python_ILI9486
Included mhs35 python library derrived from https://github.com/SirLefti/Python_ILI9486

# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

"""

import argparse
import pykms
import time
import subprocess
import digitalio
import board
from PIL import Image, ImageDraw, ImageFont
import re
import adafruit_rgb_display.st7789 as st7789  
import adafruit_rgb_display.ili9341 as ili9341 
import pyinotify
import gpiod
from gpiod.line import Direction, Value
import threading
import signal
import os
import aprslib
import math
import numpy
import mmap

def is_raspberry_pi_5():
   with open('/sys/firmware/devicetree/base/model', 'r') as f:
      model = f.read().strip()
      if "Raspberry Pi 5" in model:
          return True
      else:
          return False
ISPI5 = is_raspberry_pi_5()

def parse_arguments():
    ap = argparse.ArgumentParser()
    ap.add_argument("-l", "--log", required=True, help="Direwolf log file location")
    ap.add_argument("-f", "--fontsize", required=False, help="Font size for callsigns")
    ap.add_argument("-t", "--title_text", required=False, help="Text displayed in title bar")
    ap.add_argument("-o", "--one", action='store_true', required=False, help="Show one station at a time full screen")
    ap.add_argument("-y", "--lat", required=False, help="Your Latitude -123.4567")
    ap.add_argument("-x", "--lon", required=False, help="Your Longitude  23.4567")
    ap.add_argument("-s", "--savefile", required=False, help="Save screen updates to png file")
    ap.add_argument("-d", "--display", required=False, help="hdmi(experimental), mhs35, st7789, ili9341, ili9486")
    ap.add_argument("-m", "--mode", default="1280x720") # hdmi mode

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
   displaytype = 'hdmi'  # default to hdmi

if displaytype == 'ili9341':
   ## Large wide-screen adafruit screen 2.8" (320x240), two buttons
   spi = board.SPI()
   dc_pin = digitalio.DigitalInOut(board.D25)
   cs_pin = digitalio.DigitalInOut(board.D4)
   disp = ili9341.ILI9341(
       spi,
       cs=cs_pin,
       dc=dc_pin,
       baudrate=48000000,
       width=240,
       height=320,
       rotation=270
   )
   width = 480
   height = 320
elif displaytype == 'ili9486':
   ## Largest ili9486 display, no buttons
   from spidev import SpiDev
   import ILI9486 as ili9486
   spi = SpiDev(0,0)
   spi.mode = 0b10  
#   spi.max_speed_hz = 48000000
   spi.max_speed_hz = 32000000
   disp = ili9486.ILI9486(
       spi=spi,
       rst=25,
       dc=24,
       origin=ili9486.Origin.LOWER_RIGHT
   ).begin()
   disp.invert()
   width = 480
   height = 320
elif displaytype == 'mhs35':
   ## Large mhs35 display, no buttons, (320x480)
   from spidev import SpiDev
   from MHS35 import ILI9486
   spi = SpiDev(0,0)
   spi.mode = 0b00  
   spi.max_speed_hz = 48000000
   disp = ILI9486(spi, dc=24, rst=25, gpio_chip=0 )
   disp.begin() 
   #disp.invert()
   width = 480
   height = 320
elif displaytype == 'st7789':
   ## square adafruit screen 1.3" (240x240), two buttons
   dc_pin = digitalio.DigitalInOut(board.D25)
   cs_pin = digitalio.DigitalInOut(board.D4)
   spi = board.SPI()
   disp = st7789.ST7789(
       spi,
       cs=cs_pin,
       dc=dc_pin,
       baudrate=24000000,  #64K works
       height=240,
       width=240,
       y_offset=80,
       rotation=180
   )
   width = disp.width
   height = disp.height
elif displaytype == 'hdmi':  # default
   width = 640   # half resolution of 1280x720
   height = 360
   card = pykms.Card()
   res = pykms.ResourceManager(card)
   conn = res.reserve_connector('HDMI')
   crtc = res.reserve_crtc(conn)
   #mode = conn.get_default_mode()    # use this now that we scale in gpu
   try:
      modes = conn.get_modes()
   except:
      print("No HDMI monitor detected.  Aborting.\n")
      exit(1)
   if len(modes) == 0:
      print("Trying HDMI monitor, but no modes found.  Aborting.\n")
      exit(1)
   modehit=0
   for index,mode in enumerate(modes):
      print(mode.name + " index: " + str(index))
      if mode.name == args["mode"]:
         print("Found requested HDMI mode: " + mode.name)
         modehit=1
         break
   if modehit == 0:
      print("Unable to find requested hdmi mode.  Using smallest/last mode: " + mode.name)
   mode = modes[index]
   fb = pykms.DumbFramebuffer(card, width, height, "XR24")  
   crtc.set_mode(conn, fb, mode)
   map = fb.map(0)
   plane = res.reserve_primary_plane(crtc, pykms.PixelFormat.XRGB8888)
   modeb = mode.to_blob(card)
   req = pykms.AtomicReq(card)
   req.add(conn, "CRTC_ID", crtc.id)
   req.add(crtc, {"ACTIVE": 1,
                  "MODE_ID": modeb.id})
   r = req.commit_sync(allow_modeset = True)
   assert r == 0, "Unable to open DSI display (error %d).  Already n use?" % r
elif displaytype == 'dsi':  # default
   width = 400   # half resolution of 800x480
   height = 240
   card = pykms.Card()
   res = pykms.ResourceManager(card)
   conn = res.reserve_connector('DSI')
   crtc = res.reserve_crtc(conn)
   mode = conn.get_default_mode() # dsi only has one mode, 800x480
   # Pi5 RP1 gpu sucks too much to scale a plane, must be same as phys dimensions
   if ISPI5:
      fb = pykms.DumbFramebuffer(card, mode.hdisplay, mode.vdisplay, "XR24") # must match hardware on 5
   else:
      fb = pykms.DumbFramebuffer(card, width, height, "XR24")  
   crtc.set_mode(conn, fb, mode)
   map = fb.map(0)
   plane = res.reserve_primary_plane(crtc, pykms.PixelFormat.XRGB8888)  #Pi5
   modeb = mode.to_blob(card)
   req = pykms.AtomicReq(card)
   req.add(conn, "CRTC_ID", crtc.id)
   req.add(crtc, {"ACTIVE": 1, 
                  "MODE_ID": modeb.id})
   r = req.commit_sync(allow_modeset = True)
   assert r == 0, "Unable to open DSI display (error %d).  Already n use?" % r
else:
   print("Invalid display type specified.")
   print("ili9486, st7789, mhs35, hdmi, ili9341")
   exit(1)


# create python PIL image and drawing surface
image = Image.new("RGBA", (width, height))
draw = ImageDraw.Draw(image)

# don't write to display concurrently with thread
display_lock = threading.Lock()

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


def display_image(image, save=True):
   if displaytype == 'hdmi' or displaytype == 'dsi':
      try:
         if displaytype == 'dsi' and ISPI5:  # RP1 gpu sucks too much to scale a plane
            image = image.resize((mode.hdisplay, mode.vdisplay), Image.Resampling.BILINEAR)
            #image = image.resize((mode.hdisplay, mode.vdisplay), Image.Resampling.NEAREST)
         img_w, img_h = image.size
         buffer_size = img_w * img_h * 4   #RGBA is 4 bytes
         with mmap.mmap(fb.fd(0), buffer_size, mmap.MAP_SHARED, mmap.PROT_WRITE) as mm:
            mm.write(image.tobytes("raw","BGRA"))  # RGBA to BGRA while writing buffer
         req = pykms.AtomicReq(card)
         req.add_plane(plane, fb, crtc, None, (0, 0, mode.hdisplay, mode.vdisplay))

   
         r = req.commit_sync()
         assert r == 0, "Plane commit failed: %d" % r
   
      except Exception as e:
         print(f"Unable to write to framebuffer: {e}")
   else:
      disp.image(image)  # gpio display
   if savefile and save: image.save(savefile, compress_level=1) 


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
       display_image(image, save=True)
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
                    display_image(image, save=True)
        else:
            if bt_status == 1:
                bt_status = 0  
                bticon = Image.open('bt.small.off.png')   
                #blue_line.set_value(0) 
                blue_line.set_value(5, Value.INACTIVE)
                image.paste(bticon, (width - title_bar_height * 3 + 12  , padding + 2 ), bticon)
                with display_lock:
                    display_image(image, save=True)
        time.sleep(2)

bluetooth_thread = threading.Thread(target=bluetooth_connection_poll_thread, name="btwatch")
bluetooth_thread.start()

def redgreen_thread():                      ## change red or green status indicators and red diode
   lasttime = time.perf_counter() - 1
   last_event_was_off = False
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
            red_line.set_value(26, Value.ACTIVE)
         elif status == 'PTT 0 = 0':
            draw.ellipse(( width - title_bar_height * 2 , padding,    width - title_bar_height - padding * 2 , title_bar_height - padding), fill=(80,0,0,255))
            red_line.set_value(26, Value.INACTIVE)
         else:
            print("Unknown DCD/PTT event\n")
         with display_lock:
            display_image(image, save=False)

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

# tweak font sizes based on display resolutions
if displaytype == 'hdmi':
   bump = 26 #half of 720p, 26 is two cols on hdmi/digipeater
elif displaytype == 'dsi':
   bump = 6  
elif displaytype == 'ili9486' or displaytype == 'mhs35':
   bump = 10
else:
   bump = 0

#font(no suffix) is defined on command line, used in list mode
font = ImageFont.truetype(fontpath, fontsize)
print("fontsize: " + str(fontsize))
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
    display_image(image, save=True)
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
    display_image(image, save=True)

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
      if height >= 320: # hdmi
         symbolimage = symbolimage.resize((180, 180), Image.BILINEAR)
      image.paste(symbolimage, (0, title_bar_height), symbolimage)
      infoleftmargin = symbolimage.width + padding
      draw.text((infoleftmargin, infotopmargin),                         str(info1), font=font_small, fill="#AAAAAA")
      draw.text((infoleftmargin, infotopmargin + infolinespacing),       str(info2), font=font_small, fill="#AAAAAA")
      draw.text((infoleftmargin, infotopmargin + (infolinespacing * 2)), str(info3), font=font_small, fill="#AAAAAA")
      statustopmargin = symbolimage.height + title_bar_height 
      draw.text((5, statustopmargin), str(info4), font=font_small, fill="#AAAAAA")
      draw.text((5, height - font_epic.getbbox("J")[3] - 1), call, font=font_epic, fill="#AAAAAA") # text up from bottom edge
  
      with display_lock:
          display_image(image, save=True)
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
           display_image(image, save=False)
       time.sleep(0.1)
       draw.text((x + symbol_dimension + (symbol_dimension // 8) , y), call, font=font, fill="#AAAAAA") # start text after symbol, relative padding
       with display_lock:
           display_image(image, save=False)
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
           display_image(image, save=True)

if __name__ == '__main__':
   if args["one"]:
      single_loop()
   else:
      list_loop()
   os._exit(0)
