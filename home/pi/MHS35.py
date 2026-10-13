# ILI9486.py — MHS35b/MHS35 variant (Pi 5 compatible, lgpio)
#
# Original: SirLefti/Python_ILI9486 (Waveshare)
# Modified: craigerl/hemna — MHS35b/MHS35 support + Pi 5
#
# Key differences from original:
#   - Uses lgpio (Pi 5 compatible) instead of RPi.GPIO
#   - regwidth=16: commands AND register data sent as [0x00, byte] pairs
#   - Pixel data (bulk framebuffer writes) sent as raw bytes
#   - Pixel format: RGB565 (0x55) instead of RGB666 (0x66)
#   - image_to_data() outputs RGB565 (2 bytes/pixel)
#   - MHS35b init sequence from device tree overlay
#
# DTS source: https://github.com/SirLefti/Python_ILI9486/issues/6
# LCD wiki: https://www.lcdwiki.com/MHS-3.5inch_RPi_Display
#
# Requirements:
#   pip install lgpio spidev numpy pillow
#
# Usage:
#   from spidev import SpiDev
#   from MHS35 import ILI9486
#
#   spi = SpiDev()
#   spi.open(0, 0)
#   spi.max_speed_hz = 16000000
#   spi.mode = 0
#   d = ILI9486(spi, dc=24, rst=25)
#   d.begin()
#   d.clear((255, 0, 0))
#   d.display()
#
# Copyright (c)
# Authors: Tony DiCola, Liqun Hu, Thorben Yzer
# MHS35b modifications: craigerl / hemna
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
from enum import Enum
import time
import numpy as np
from PIL import Image, ImageDraw
import lgpio
from spidev import SpiDev

# constants
LCD_WIDTH = 320
LCD_HEIGHT = 480

# commands
CMD_RDPXLFMT = 0x0C

CMD_SLPIN = 0x10
CMD_SLPOUT = 0x11

CMD_INVOFF = 0x20
CMD_INVON = 0x21
CMD_DISPOFF = 0x28
CMD_DISPON = 0x29

CMD_SETCA = 0x2A
CMD_SETPA = 0x2B
CMD_WRMEM = 0x2C
CMD_RDMEM = 0x2E

CMD_MADCTL = 0x36
CMD_IDLOFF = 0x38
CMD_IDLON = 0x39
CMD_PXLFMT = 0x3A

CMD_IFMODE = 0xB0

CMD_PWRCTLNOR = 0xC2
CMD_VCOMCTL = 0xC5

CMD_PGAMCTL = 0xE0
CMD_NGAMCTL = 0xE1


def image_to_data(image: Image) -> list:
    """Converts a PIL image to RGB565 format (16-bit, 2 bytes per pixel).

    RGB565 packing:
        Byte 1 (high): RRRRRGGG
        Byte 2 (low):  GGGBBBBB
    """
    pb = np.array(image.convert('RGB')).astype('uint16')
    r = pb[:, :, 0]
    g = pb[:, :, 1]
    b = pb[:, :, 2]

    # Pack into RGB565: 5 bits red, 6 bits green, 5 bits blue
    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

    # Split 16-bit values into high byte and low byte
    high = ((rgb565 >> 8) & 0xFF).astype('uint8')
    low = (rgb565 & 0xFF).astype('uint8')

    # Interleave high and low bytes: [H0, L0, H1, L1, ...]
    result = np.empty((rgb565.shape[0], rgb565.shape[1], 2), dtype='uint8')
    result[:, :, 0] = high
    result[:, :, 1] = low

    return result.flatten().tolist()


class Origin(Enum):
    """Representation of the display origin."""

    # data: MY | MX | MV | ML | BGR | MH | X | X
    UPPER_LEFT = 0x28
    UPPER_LEFT_MIRRORED = 0xA8
    LOWER_LEFT = 0x48
    LOWER_LEFT_MIRRORED = 0x08
    UPPER_RIGHT = 0x88
    UPPER_RIGHT_MIRRORED = 0xC8
    LOWER_RIGHT = 0xE8
    LOWER_RIGHT_MIRRORED = 0x68


class ILI9486:
    """ILI9486 TFT LCD driver — MHS35b/MHS35 variant.

    Uses lgpio (Pi 5 compatible).
    16-bit register interface: commands and register data are sent as [0x00, byte].
    Pixel data (bulk framebuffer writes after WRMEM) is sent as raw bytes.
    """

    @classmethod
    def landscape_dimensions(cls) -> tuple:
        return LCD_HEIGHT, LCD_WIDTH

    @classmethod
    def portrait_dimensions(cls) -> tuple:
        return LCD_WIDTH, LCD_HEIGHT

    def __init__(self, spi: SpiDev, dc: int, rst: int = None, *,
                 origin: Origin = Origin.UPPER_LEFT, gpio_chip: int = 4):
        """
        Args:
            spi: SpiDev instance (already opened with spi.open())
            dc: BCM GPIO pin number for Data/Command
            rst: BCM GPIO pin number for Reset (optional)
            origin: Display orientation (default UPPER_LEFT = landscape)
            gpio_chip: GPIO chip number (Pi 5 = 4, Pi 4/3 = 0)
        """
        self.__spi = spi
        self.__dc = dc
        self.__rst = rst
        self.__origin = origin
        self.__width = LCD_WIDTH
        self.__height = LCD_HEIGHT
        self.__inverted = False
        self.__idle = False

        # Open GPIO chip
        self.__gpio = lgpio.gpiochip_open(gpio_chip)
        lgpio.gpio_claim_output(self.__gpio, self.__dc, 1)
        if self.__rst is not None:
            lgpio.gpio_claim_output(self.__gpio, self.__rst, 1)

        # Swap width and height if landscape mode (MV bit set)
        if self.__origin.value & 0x20:
            self.__width, self.__height = self.__height, self.__width
        self.__buffer = Image.new('RGB', (self.__width, self.__height), (0, 0, 0))

    def __del__(self):
        try:
            lgpio.gpiochip_close(self.__gpio)
        except Exception:
            pass

    def dimensions(self) -> tuple:
        return self.__width, self.__height

    def is_landscape(self) -> bool:
        return bool(self.__origin.value & 0x20)

    def command(self, data):
        """Send command byte: DC low, 16-bit padded [0x00, cmd]."""
        lgpio.gpio_write(self.__gpio, self.__dc, 0)
        if isinstance(data, int):
            self.__spi.writebytes([0x00, data])
        else:
            for byte in data:
                self.__spi.writebytes([0x00, byte])
        return self

    def data(self, data):
        """Send register data: DC high, 16-bit padded [0x00, byte] per byte."""
        lgpio.gpio_write(self.__gpio, self.__dc, 1)
        if isinstance(data, int):
            self.__spi.writebytes([0x00, data])
        else:
            for byte in data:
                self.__spi.writebytes([0x00, byte])
        return self

    def data_bulk(self, data, chunk_size=4096):
        """Send pixel data: DC high, raw bytes (no 16-bit padding)."""
        lgpio.gpio_write(self.__gpio, self.__dc, 1)
        if isinstance(data, list):
            for start in range(0, len(data), chunk_size):
                end = min(start + chunk_size, len(data))
                self.__spi.writebytes(data[start:end])
        else:
            self.__spi.writebytes(data)
        return self

    def reset(self):
        """Resets the display if a reset pin is provided."""
        if self.__rst is not None:
            lgpio.gpio_write(self.__gpio, self.__rst, 1)
            time.sleep(.001)
            lgpio.gpio_write(self.__gpio, self.__rst, 0)
            time.sleep(.001)
            lgpio.gpio_write(self.__gpio, self.__rst, 1)
            time.sleep(.120)
            self.__inverted = False
            self.__idle = False
        return self

    def _init_sequence(self):
        """MHS35b/MHS35 initialization sequence from device tree overlay.

        DTS init (mhs35-overlay.dts, tft35a-overlay, mhs35b-overlay all identical):
          0x10000f1 0x36 0x04 0x00 0x3c 0x0f 0x8f
          0x10000f2 0x18 0xa3 0x12 0x02 0xb2 0x12 0xff 0x10 0x00
          0x10000f8 0x21 0x04
          0x10000f9 0x00 0x08
          0x1000036 0x08
          0x10000b4 0x00
          0x10000c1 0x41
          0x10000c5 0x00 0x91 0x80 0x00
          0x10000e0 ... (15 bytes positive gamma)
          0x10000e1 ... (15 bytes negative gamma)
          0x100003a 0x55
          0x1000011
          0x1000036 0x28
          0x20000ff
          0x1000029
        """
        # Manufacturer-specific registers
        self.command(0xF1).data([0x36, 0x04, 0x00, 0x3C, 0x0F, 0x8F])
        self.command(0xF2).data([0x18, 0xA3, 0x12, 0x02, 0xB2, 0x12, 0xFF, 0x10, 0x00])
        self.command(0xF8).data([0x21, 0x04])
        self.command(0xF9).data([0x00, 0x08])

        # MADCTL initial
        self.command(CMD_MADCTL).data(0x08)

        # Display Inversion Control
        self.command(0xB4).data(0x00)

        # Power Control 2
        self.command(0xC1).data(0x41)

        # VCOM Control
        self.command(CMD_VCOMCTL).data([0x00, 0x91, 0x80, 0x00])

        # Positive Gamma Control
        self.command(CMD_PGAMCTL).data([0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98,
                                        0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00])

        # Negative Gamma Control
        self.command(CMD_NGAMCTL).data([0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75,
                                        0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00])

        # Pixel Format — RGB565
        self.command(CMD_PXLFMT).data(0x55)

        # Sleep Out
        self.command(CMD_SLPOUT)

        # MADCTL — final origin
        self.command(CMD_MADCTL).data(self.__origin.value)

        # Delay 255ms
        time.sleep(0.255)

        # Display On
        self.command(CMD_DISPON)
        return self

    def begin(self):
        """Initializes the display by resetting it and calling the init sequence."""
        return self.reset()._init_sequence()

    def set_window(self, x0=0, y0=0, x1=None, y1=None):
        """Sets the pixel address window for proceeding drawing commands."""
        if x1 is None:
            x1 = self.__width - 1
        if y1 is None:
            y1 = self.__height - 1
        self.command(CMD_SETCA)
        self.data([x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF])
        self.command(CMD_SETPA)
        self.data([y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF])
        return self

#    def display(self, image=None, x0=0, y0=0):
    def image(self, image=None, x0=0, y0=0):
        """Writes the display buffer or provided image to the display."""
        if image is None:
            image = self.__buffer
        width, height = image.size
        x1 = x0 + width - 1
        y1 = y0 + height - 1
#craiger        if image.mode != 'RGB':
#craiger            raise ValueError('Image must be in RGB format')
        if x1 >= self.__width or y1 >= self.__height or x0 < 0 or y0 < 0:
            raise ValueError(
                'Image exceeds display bounds ({0}x{1})'.format(self.__width, self.__height))
        self.set_window(x0, y0, x1, y1)
        # Send pixel data as raw bulk bytes (not 16-bit padded)
        data = image_to_data(image)
        self.command(CMD_WRMEM)
        self.data_bulk(data)
        return self

    def clear(self, color=(0, 0, 0)):
        """Clears the image buffer to the specified RGB color or black if not provided."""
        width, height = self.__buffer.size
        self.__buffer.putdata([color] * (width * height))
        return self

    def draw(self) -> ImageDraw:
        """Returns a PIL ImageDraw instance for 2D drawing on the image buffer."""
        return ImageDraw.Draw(self.__buffer)

#    def image(self) -> Image:
#        """Returns the image buffer."""
#        return self.__buffer

    def is_inverted(self) -> bool:
        return self.__inverted

    def invert(self, state: bool = True):
        if state:
            self.command(CMD_INVON)
        else:
            self.command(CMD_INVOFF)
        self.__inverted = state
        return self

    def is_idle(self) -> bool:
        return self.__idle

    def idle(self, state: bool = True):
        if state:
            self.command(CMD_IDLON)
        else:
            self.command(CMD_IDLOFF)
        self.__idle = state
        return self

    def on(self):
        return self.command(CMD_DISPON)

    def off(self):
        return self.command(CMD_DISPOFF)

    def sleep(self):
        self.command(CMD_SLPIN)
        time.sleep(0.005)
        return self

    def wake_up(self):
        self.command(CMD_SLPOUT)
        time.sleep(0.005)
        return self
