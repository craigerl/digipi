#!/usr/bin/python3

# digibanner.py

"""
Craig Lamparter KM6LYW,  2025, MIT Licnese

This will display a splash screen on an ST7789 or ILI9341 displays.

Usage:

digibanner.py --big="big text on top" --small="small text under that"  --tiny="tiny text at bottom" --graphic="icon.png" --display="st7789"

Do not install the kernel module/framebuffer.

Uncomment one of the three display definitions below depending
on your particular monitor.  Default is the small ST7789/240x240 display.
   st7789
   ili9341
   ili9486
   mhs35
   hdmi
   dsi


See the top of direwatch.py for installation instructions and dependencies

# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT
"""

import argparse
import digitalio
import board
from PIL import Image, ImageDraw, ImageFont
import adafruit_rgb_display.st7789 as st7789  
import adafruit_rgb_display.ili9341 as ili9341 
#import ILI9486 as ili9486
import os
import subprocess
import numpy
import fcntl
import struct

# Configuration for CS and DC pins (these are PiTFT defaults):
#dc_pin = digitalio.DigitalInOut(board.D25)
#cs_pin = digitalio.DigitalInOut(board.D4)

# Setup SPI bus using hardware SPI:
#spi = board.SPI()

# define some constants to help with graphics layout
padding = 4 
title_bar_height = 34

def parse_arguments():
    ap = argparse.ArgumentParser()
    ap.add_argument("-f", "--fontsize", required=False, help="Font size for callsigns")
    ap.add_argument("-b", "--big", required=False, help="large text to display")
    ap.add_argument("-s", "--small", required=False, help="smaller text underneath")
    ap.add_argument("-t", "--tiny", required=False, help="tiny text underneath smaller text")
    ap.add_argument("-g", "--graphic", required=False, help="Display a small image on screen")
    ap.add_argument("-d", "--display", required=False, help="st7789, ili9341, ili9486, mhs35, hdmi, dsi")
    args = vars(ap.parse_args())
    return args

def get_fb0_resolution():
    try:
        # Read virtual_size or current_size
        with open("/sys/class/graphics/fb0/virtual_size", "r") as f:
            size_str = f.read().strip()
        
        if size_str == "0,0":
            # If virtual_size is 0,0, try current_size
            with open("/sys/class/graphics/fb0/current_size", "r") as f:
                size_str = f.read().strip()
                
        w, h = map(int, size_str.split(","))
        return w, h
    except FileNotFoundError:
        print("Error: /sys/class/graphics/fb0 not found. Is fb0 active?")
        return None
    except ValueError:
        print("Error: Could not parse resolution values.")
        return None

args = parse_arguments()

if args["fontsize"]:
   fontsize = int(args["fontsize"])
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

if args["graphic"]:
   graphic = args["graphic"]

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
   fontbump = 4
elif displaytype == 'ili9486':
   ## Large Waveshare ili9486 display, no buttons, (320x480)
   from spidev import SpiDev
   import ILI9486 as ili9486
   spi = SpiDev(0,0)
   spi.mode = 0b10
   spi.max_speed_hz = 32000000
   disp = ili9486.ILI9486(
       spi=spi,
       rst=25,
       dc=24,
       origin=ili9486.Origin.LOWER_RIGHT
   ).begin()
   disp.invert()
   fontbump = 10
elif displaytype == 'mhs35':
   ## Large mhs35 display, no buttons, (320x480)
   from spidev import SpiDev
   from MHS35 import ILI9486
   spi = SpiDev(0,0)
   spi.mode = 0b00
   spi.max_speed_hz = 48000000
   disp = ILI9486(spi, dc=24, rst=25, gpio_chip=0 )
   disp.begin()
#  disp.invert()
   fontbump = 10
elif displaytype == 'st7789':
   ## square adafruit screen 1.3" (240x240), two buttons
   dc_pin = digitalio.DigitalInOut(board.D25)
   cs_pin = digitalio.DigitalInOut(board.D4)
   spi = board.SPI()
   disp = st7789.ST7789(
      spi,
      cs=cs_pin,
      dc=dc_pin,
      baudrate=24000000,
      height=240,
      width=240,
      y_offset=80,
      rotation=180
   )
   fontbump = 0
elif displaytype == 'hdmi':  # default
   pass
   fontbump = 10
elif displaytype == 'dsi':  # default
   pass
   fontbump = 10
else:
   print("Invalid display type specified.")
   print("ili9486, st7789, mhs35, hdmi, dsi, ili9341")
   exit(0)


FBIOGET_VSCREENINFO = 0x4600
FBIOGET_FSCREENINFO = 0x4602


def _get_fb_info():
    fd = os.open('/dev/fb0', os.O_RDWR)
    try:
        vbuf = bytearray(236)
        fcntl.ioctl(fd, FBIOGET_VSCREENINFO, vbuf)
        w = struct.unpack_from('<i', vbuf, 0)[0]
        h = struct.unpack_from('<i', vbuf, 4)[0]
        bpp = struct.unpack_from('<i', vbuf, 24)[0]

        fbuf = bytearray(66)
        fcntl.ioctl(fd, FBIOGET_FSCREENINFO, fbuf)
        line_length = struct.unpack_from('<I', fbuf, 44)[0]

        return w, h, bpp, line_length
    finally:
        os.close(fd)

def display_image(image, save=True):
    if displaytype in ('hdmi', 'dsi'):
        try:
            w, h, bpp, line_length = _get_fb_info()

            img = image.resize((w, h), Image.Resampling.BILINEAR)
            rgb = numpy.array(img.convert('RGB'), dtype=numpy.uint32)

            if bpp == 16:   # not pi5
                pixels = (((rgb[:, :, 0] >> 3) << 11) |
                          ((rgb[:, :, 1] >> 2) << 5) |
                          (rgb[:, :, 2] >> 3)).astype(numpy.uint16)
                dtype, bpp_bytes = numpy.uint16, 2
            elif bpp == 32:  # pi5
                pixels = ((rgb[:, :, 0] << 16) |
                          (rgb[:, :, 1] << 8) |
                          rgb[:, :, 2]).astype(numpy.uint32)
                dtype, bpp_bytes = numpy.uint32, 4
            else:
                raise ValueError(f"Unsupported bpp: {bpp}")

            stride = line_length // bpp_bytes
            fb = numpy.memmap('/dev/fb0', dtype=dtype, mode='w+', shape=(h, stride))
            fb[:, :w] = pixels
            fb.flush()
        except Exception as e:
            print(f"Unable to write to framebuffer: {e}")
    else:
        disp.image(image)

    if save:
        image.save('/run/direwatch.png', compress_level=1)


# Create image and drawing object
if displaytype == 'st7789' or displaytype == 'ili9341':
   if disp.rotation % 180 == 90:
       height = disp.width
       width = disp.height
   else:
       width = disp.width
       height = disp.height
elif displaytype == 'ili9486' or displaytype == 'mhs35':
   width=480
   height=320
elif displaytype == 'hdmi':
#   width=320
#   height=240
   width=640
   height=360
#   width=640
#   height=360
elif displaytype == 'dsi':
   width=400
   height=240
else:                         # sane default
   width=240
   height=240

image = Image.new("RGB", (width, height))
draw = ImageDraw.Draw(image)

font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
line_height = font.getbbox("ABCJQ")[3] - 1          # tallest callsign, with dangling J/Q tails
font_huge = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 34 + fontbump)
#font_big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 22 + fontbump)
font_big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf", 22 + fontbump)
font_tiny = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18 + fontbump)

# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=(0,0,0), fill="#000000")

if args["graphic"]:
   background = Image.open(args["graphic"])
   image.paste(background, (width - 120, 37), background)

draw.rectangle((0, 0, width, 30), outline=(0,0,0), fill=(30,30,30))
draw.text(   (10  ,  0  ) , "DigiPi", font=font, fill="#999999")
draw.text(   (10  ,  height * .28 )                                   , big,   font=font_huge, fill="#888888")
draw.text(   (10  ,  height * .28 + font_big.getbbox("BgJ")[3] + 12 ) , small, font=font_big,  fill="#666666")
draw.text(   (10  ,  height       - font_big.getbbox("BgJ")[3] - 4  ) , tiny,  font=font_tiny, fill="#666666")
display_image(image, save=True)

exit(0)

