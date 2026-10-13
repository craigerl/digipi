# SPDX-FileCopyrightText: 2017 Radomir Dopieralski for Adafruit Industries
# SPDX-FileCopyrightText: 2023 Matt Land
#
# SPDX-License-Identifier: MIT

"""
`adafruit_rgb_display.ili9341`
====================================================

A simple driver for the ILI9341/ILI9340-based displays.

* Author(s): Radomir Dopieralski, Michael McWethy, Matt Land
"""
import struct

from adafruit_rgb_display.rgb import DisplaySPI

try:
    from typing import Optional
    import digitalio
    import busio
except ImportError:
    pass

__version__ = "3.12.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_RGB_Display.git"


class ILI9341(DisplaySPI):
    """
    A simple driver for the ILI9341/ILI9340-based displays.

    >>> import busio
    >>> import digitalio
    >>> import board
    >>> from adafruit_rgb_display import color565
    >>> import adafruit_rgb_display.ili9341 as ili9341
    >>> spi = busio.SPI(clock=board.SCK, MOSI=board.MOSI, MISO=board.MISO)
    >>> display = ili9341.ILI9341(spi, cs=digitalio.DigitalInOut(board.GPIO0),
    ...    dc=digitalio.DigitalInOut(board.GPIO15))
    >>> display.fill(color565(0xff, 0x11, 0x22))
    >>> display.pixel(120, 160, 0)
    """

    _COLUMN_SET = 0x2A
    _PAGE_SET = 0x2B
    _RAM_WRITE = 0x2C
    _RAM_READ = 0x2E
    _INIT = (
        (0xEF, b"\x03\x80\x02"),
        (0xCF, b"\x00\xc1\x30"),
        (0xED, b"\x64\x03\x12\x81"),
        (0xE8, b"\x85\x00\x78"),
        (0xCB, b"\x39\x2c\x00\x34\x02"),
        (0xF7, b"\x20"),
        (0xEA, b"\x00\x00"),
        (0xC0, b"\x23"),  # Power Control 1, VRH[5:0]
        (0xC1, b"\x10"),  # Power Control 2, SAP[2:0], BT[3:0]
        (0xC5, b"\x3e\x28"),  # VCM Control 1
        (0xC7, b"\x86"),  # VCM Control 2
        (0x36, b"\x48"),  # Memory Access Control
        (0x3A, b"\x55"),  # Pixel Format
        (0xB1, b"\x00\x18"),  # FRMCTR1
        (0xB6, b"\x08\x82\x27"),  # Display Function Control
        (0xF2, b"\x00"),  # 3Gamma Function Disable
        (0x26, b"\x01"),  # Gamma Curve Selected
        (
            0xE0,  # Set Gamma
            b"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00",
        ),
        (
            0xE1,  # Set Gamma
            b"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f",
        ),
        (0x11, None),
        (0x29, None),
    )
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"
    _DECODE_PIXEL = ">BBB"

    # pylint: disable-msg=too-many-arguments
    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 240,
        height: int = 320,
        baudrate: int = 16000000,
        polarity: int = 0,
        phase: int = 0,
        rotation: int = 0,
    ):
        super().__init__(
            spi,
            dc,
            cs,
            rst=rst,
            width=width,
            height=height,
            baudrate=baudrate,
            polarity=polarity,
            phase=phase,
            rotation=rotation,
        )
        self._scroll = 0

    # pylint: enable-msg=too-many-arguments

    def scroll(
        self, dy: Optional[int] = None  # pylint: disable-msg=invalid-name
    ) -> Optional[int]:
        """Scroll the display by delta y"""
        if dy is None:
            return self._scroll
        self._scroll = (self._scroll + dy) % self.height
        self.write(0x37, struct.pack(">H", self._scroll))
        return None
