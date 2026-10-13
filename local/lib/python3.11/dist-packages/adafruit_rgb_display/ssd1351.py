# SPDX-FileCopyrightText: 2017 Radomir Dopieralski for Adafruit Industries
# SPDX-FileCopyrightText: 2023 Matt Land
#
# SPDX-License-Identifier: MIT

"""
`adafruit_rgb_display.ssd1351`
====================================================

A simple driver for the SSD1351-based displays.

* Author(s): Radomir Dopieralski, Michael McWethy, Matt Land
"""
from micropython import const
from adafruit_rgb_display.rgb import DisplaySPI

try:
    from typing import Optional
    import digitalio
    import busio
except ImportError:
    pass

__version__ = "3.12.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_RGB_Display.git"

_SETCOLUMN = const(0x15)
_SETROW = const(0x75)
_WRITERAM = const(0x5C)
_READRAM = const(0x5D)
_SETREMAP = const(0xA0)
_STARTLINE = const(0xA1)
_DISPLAYOFFSET = const(0xA2)
_DISPLAYALLOFF = const(0xA4)
_DISPLAYALLON = const(0xA5)
_NORMALDISPLAY = const(0xA6)
_INVERTDISPLAY = const(0xA7)
_FUNCTIONSELECT = const(0xAB)
_DISPLAYOFF = const(0xAE)
_DISPLAYON = const(0xAF)
_PRECHARGE = const(0xB1)
_DISPLAYENHANCE = const(0xB2)
_CLOCKDIV = const(0xB3)
_SETVSL = const(0xB4)
_SETGPIO = const(0xB5)
_PRECHARGE2 = const(0xB6)
_SETGRAY = const(0xB8)
_USELUT = const(0xB9)
_PRECHARGELEVEL = const(0xBB)
_VCOMH = const(0xBE)
_CONTRASTABC = const(0xC1)
_CONTRASTMASTER = const(0xC7)
_MUXRATIO = const(0xCA)
_COMMANDLOCK = const(0xFD)
_HORIZSCROLL = const(0x96)
_STOPSCROLL = const(0x9E)
_STARTSCROLL = const(0x9F)


class SSD1351(DisplaySPI):
    """
    A simple driver for the SSD1351-based displays.

    >>> import busio
    >>> import digitalio
    >>> import board
    >>> from adafruit_rgb_display import color565
    >>> import adafruit_rgb_display.ssd1351 as ssd1351
    >>> spi = busio.SPI(clock=board.SCK, MOSI=board.MOSI, MISO=board.MISO)
    >>> display = ssd1351.SSD1351(spi, cs=digitalio.DigitalInOut(board.GPIO0),
    ...    dc=digitalio.DigitalInOut(board.GPIO15), rst=digitalio.DigitalInOut(board.GPIO16))
    >>> display.fill(0x7521)
    >>> display.pixel(32, 32, 0)
    """

    _COLUMN_SET = _SETCOLUMN
    _PAGE_SET = _SETROW
    _RAM_WRITE = _WRITERAM
    _RAM_READ = _READRAM
    _INIT = (
        (_COMMANDLOCK, b"\x12"),
        (_COMMANDLOCK, b"\xb1"),
        (_DISPLAYOFF, b""),
        (_DISPLAYENHANCE, b"\xa4\x00\x00"),
        # 7:4 = Oscillator Frequency,
        # 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
        (_CLOCKDIV, b"\xf0"),
        (_MUXRATIO, b"\x7f"),  # 127
        (_SETREMAP, b"\x74"),
        (_STARTLINE, b"\x00"),
        (_DISPLAYOFFSET, b"\x00"),
        (_SETGPIO, b"\x00"),
        (_FUNCTIONSELECT, b"\x01"),
        (_PRECHARGE, b"\x32"),
        (_PRECHARGELEVEL, b"\x1f"),
        (_VCOMH, b"\x05"),
        (_NORMALDISPLAY, b""),
        (_CONTRASTABC, b"\xc8\x80\xc8"),
        (_CONTRASTMASTER, b"\x0a"),
        (_SETVSL, b"\xa0\xb5\x55"),
        (_PRECHARGE2, b"\x01"),
        (_DISPLAYON, b""),
    )
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">BB"

    # pylint: disable-msg=useless-super-delegation, too-many-arguments
    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 128,
        height: int = 128,
        baudrate: int = 16000000,
        polarity: int = 0,
        phase: int = 0,
        *,
        x_offset: int = 0,
        y_offset: int = 0,
        rotation: int = 0
    ):
        super().__init__(
            spi,
            dc,
            cs,
            rst,
            width,
            height,
            baudrate=min(baudrate, 16000000),  # Limit to Display Max Baudrate
            polarity=polarity,
            phase=phase,
            x_offset=x_offset,
            y_offset=y_offset,
            rotation=rotation,
        )
