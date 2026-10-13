# SPDX-FileCopyrightText: 2025 Liz Clark for Adafruit Industries
#
# SPDX-License-Identifier: MIT
"""
`adafruit_rgb_display.gc9a01a`
====================================================
A simple driver for the GC9A01A-based displays.

* Author(s): Liz Clark

"""

import time

import busio
import digitalio
from micropython import const

from adafruit_rgb_display.rgb import DisplaySPI

try:
    from typing import Optional
except ImportError:
    pass

__version__ = "3.14.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_RGB_Display.git"

# Command constants
_SWRESET = const(0xFE)
_SLPOUT = const(0x11)
_NORON = const(0x13)
_INVOFF = const(0x20)
_INVON = const(0x21)
_DISPOFF = const(0x28)
_DISPON = const(0x29)
_CASET = const(0x2A)
_RASET = const(0x2B)
_RAMWR = const(0x2C)
_RAMRD = const(0x2E)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)


class GC9A01A(DisplaySPI):
    """
    A simple driver for the GC9A01A-based displays.

    >>> import busio
    >>> import digitalio
    >>> import board
    >>> from adafruit_rgb_display import gc9a01a
    >>> spi = busio.SPI(clock=board.SCK, MOSI=board.MOSI, MISO=board.MISO)
    >>> display = gc9a01a.GC9A01A(spi, cs=digitalio.DigitalInOut(board.CE0),
    ...    dc=digitalio.DigitalInOut(board.D25), rst=digitalio.DigitalInOut(board.D27))
    >>> display.fill(0x7521)
    >>> display.pixel(64, 64, 0)
    """

    _COLUMN_SET = _CASET
    _PAGE_SET = _RASET
    _RAM_WRITE = _RAMWR
    _RAM_READ = _RAMRD
    _INIT = (
        (_SWRESET, None),
        (0xEF, None),  # Inter Register Enable2
        (0xB6, b"\x00\x00"),  # Display Function Control
        (_MADCTL, b"\x48"),  # Memory Access Control - Set to BGR color filter panel
        (_COLMOD, b"\x05"),  # Interface Pixel Format - 16 bits per pixel
        (0xC3, b"\x13"),  # Power Control 2
        (0xC4, b"\x13"),  # Power Control 3
        (0xC9, b"\x22"),  # Power Control 4
        (0xF0, b"\x45\x09\x08\x08\x26\x2a"),  # SET_GAMMA1
        (0xF1, b"\x43\x70\x72\x36\x37\x6f"),  # SET_GAMMA2
        (0xF2, b"\x45\x09\x08\x08\x26\x2a"),  # SET_GAMMA3
        (0xF3, b"\x43\x70\x72\x36\x37\x6f"),  # SET_GAMMA4
        (0x66, b"\x3c\x00\xcd\x67\x45\x45\x10\x00\x00\x00"),
        (0x67, b"\x00\x3c\x00\x00\x00\x01\x54\x10\x32\x98"),
        (0x74, b"\x10\x85\x80\x00\x00\x4e\x00"),
        (0x98, b"\x3e\x07"),
        (0x35, None),  # Tearing Effect Line ON
        (_INVON, None),  # Display Inversion ON
        (_SLPOUT, None),  # Sleep Out Mode
        (_NORON, None),  # Normal Display Mode ON
        (_DISPON, None),  # Display ON
    )

    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 240,
        height: int = 240,
        baudrate: int = 24000000,
        polarity: int = 0,
        phase: int = 0,
        *,
        x_offset: int = 0,
        y_offset: int = 0,
        rotation: int = 0,
    ) -> None:
        super().__init__(
            spi,
            dc,
            cs,
            rst,
            width,
            height,
            baudrate=baudrate,
            polarity=polarity,
            phase=phase,
            x_offset=x_offset,
            y_offset=y_offset,
            rotation=rotation,
        )

    def init(self) -> None:
        """Initialize the display."""
        if self.rst:
            self.rst.value = 0
            time.sleep(0.05)
            self.rst.value = 1
            time.sleep(0.05)

        super().init()
