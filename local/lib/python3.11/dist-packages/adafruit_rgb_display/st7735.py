# SPDX-FileCopyrightText: 2017 Radomir Dopieralski for Adafruit Industries
# SPDX-FileCopyrightText: 2023 Matt Land
#
# SPDX-License-Identifier: MIT

"""
`adafruit_rgb_display.st7735`
====================================================

A simple driver for the ST7735-based displays.

* Author(s): Radomir Dopieralski, Michael McWethy, Matt Land
"""
import struct

from micropython import const
from adafruit_rgb_display.rgb import DisplaySPI

try:
    from typing import Optional, Tuple, ByteString, Union
    import digitalio
    import busio
except ImportError:
    pass

__version__ = "3.12.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_RGB_Display.git"

_NOP = const(0x00)
_SWRESET = const(0x01)
_RDDID = const(0x04)
_RDDST = const(0x09)

_SLPIN = const(0x10)
_SLPOUT = const(0x11)
_PTLON = const(0x12)
_NORON = const(0x13)

_INVOFF = const(0x20)
_INVON = const(0x21)
_DISPOFF = const(0x28)
_DISPON = const(0x29)
_CASET = const(0x2A)
_RASET = const(0x2B)
_RAMWR = const(0x2C)
_RAMRD = const(0x2E)

_PTLAR = const(0x30)
_COLMOD = const(0x3A)
_MADCTL = const(0x36)

_FRMCTR1 = const(0xB1)
_FRMCTR2 = const(0xB2)
_FRMCTR3 = const(0xB3)
_INVCTR = const(0xB4)
_DISSET5 = const(0xB6)

_PWCTR1 = const(0xC0)
_PWCTR2 = const(0xC1)
_PWCTR3 = const(0xC2)
_PWCTR4 = const(0xC3)
_PWCTR5 = const(0xC4)
_VMCTR1 = const(0xC5)

_RDID1 = const(0xDA)
_RDID2 = const(0xDB)
_RDID3 = const(0xDC)
_RDID4 = const(0xDD)

_PWCTR6 = const(0xFC)

_GMCTRP1 = const(0xE0)
_GMCTRN1 = const(0xE1)


class ST7735(DisplaySPI):
    """
    A simple driver for the ST7735-based displays.

    >>> import busio
    >>> import digitalio
    >>> import board
    >>> from adafruit_rgb_display import color565
    >>> import adafruit_rgb_display.st7735 as st7735
    >>> spi = busio.SPI(clock=board.SCK, MOSI=board.MOSI, MISO=board.MISO)
    >>> display = st7735.ST7735(spi, cs=digitalio.DigitalInOut(board.GPIO0),
    ...    dc=digitalio.DigitalInOut(board.GPIO15), rst=digitalio.DigitalInOut(board.GPIO16))
    >>> display.fill(0x7521)
    >>> display.pixel(64, 64, 0)
    """

    _COLUMN_SET = _CASET
    _PAGE_SET = _RASET
    _RAM_WRITE = _RAMWR
    _RAM_READ = _RAMRD
    _INIT = (
        (_SWRESET, None),
        (_SLPOUT, None),
        (_COLMOD, b"\x05"),  # 16bit color
        # fastest refresh, 6 lines front porch, 3 line back porch
        (_FRMCTR1, b"\x00\x06\x03"),
        (_MADCTL, b"\x08"),  # bottom to top refresh
        # 1 clk cycle nonoverlap, 2 cycle gate rise, 3 sycle osc equalie,
        # fix on VTL
        (_DISSET5, b"\x15\x02"),
        (_INVCTR, b"0x00"),  # line inversion
        (_PWCTR1, b"\x02\x70"),  # GVDD = 4.7V, 1.0uA
        (_PWCTR2, b"\x05"),  # VGH=14.7V, VGL=-7.35V
        (_PWCTR3, b"\x01\x02"),  # Opamp current small, Boost frequency
        (_VMCTR1, b"\x3c\x38"),  # VCOMH = 4V, VOML = -1.1V
        (_PWCTR6, b"\x11\x15"),
        (
            _GMCTRP1,
            b"\x09\x16\x09\x20\x21\x1b\x13\x19" b"\x17\x15\x1e\x2b\x04\x05\x02\x0e",
        ),  # Gamma
        (
            _GMCTRN1,
            b"\x08\x14\x08\x1e\x22\x1d\x18\x1e" b"\x18\x1a\x24\x2b\x06\x06\x02\x0f",
        ),
        (_CASET, b"\x00\x02\x00\x81"),  # XSTART = 2, XEND = 129
        (_RASET, b"\x00\x02\x00\x81"),  # XSTART = 2, XEND = 129
        (_NORON, None),
        (_DISPON, None),
    )  # type: Tuple[Tuple[int, Union[ByteString, None]], ...]
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"

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


class ST7735R(ST7735):
    """A simple driver for the ST7735R-based displays."""

    _INIT = (
        (_SWRESET, None),
        (_SLPOUT, None),
        (_MADCTL, b"\xc8"),
        (_COLMOD, b"\x05"),  # 16bit color
        (_INVCTR, b"\x07"),
        (_FRMCTR1, b"\x01\x2c\x2d"),
        (_FRMCTR2, b"\x01\x2c\x2d"),
        (_FRMCTR3, b"\x01\x2c\x2d\x01\x2c\x2d"),
        (_PWCTR1, b"\x02\x02\x84"),
        (_PWCTR2, b"\xc5"),
        (_PWCTR3, b"\x0a\x00"),
        (_PWCTR4, b"\x8a\x2a"),
        (_PWCTR5, b"\x8a\xee"),
        (_VMCTR1, b"\x0e"),
        (_INVOFF, None),
        (
            _GMCTRP1,
            b"\x02\x1c\x07\x12\x37\x32\x29\x2d" b"\x29\x25\x2B\x39\x00\x01\x03\x10",
        ),  # Gamma
        (
            _GMCTRN1,
            b"\x03\x1d\x07\x06\x2E\x2C\x29\x2D" b"\x2E\x2E\x37\x3F\x00\x00\x02\x10",
        ),
    )

    # pylint: disable-msg=useless-super-delegation, too-many-arguments
    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 128,
        height: int = 160,
        baudrate: int = 16000000,
        polarity: int = 0,
        phase: int = 0,
        *,
        x_offset: int = 0,
        y_offset: int = 0,
        rotation: int = 0,
        bgr: bool = False,
        invert: bool = False,
    ) -> None:
        self._bgr = bgr
        self._invert = invert
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
        super().init()
        cols = struct.pack(">HH", 0, self.width - 1)
        rows = struct.pack(">HH", 0, self.height - 1)

        for command, data in (
            (_CASET, cols),
            (_RASET, rows),
            (_NORON, None),
            (_DISPON, None),
        ):
            self.write(command, data)
        if self._bgr:
            self.write(_MADCTL, b"\xc0")
        if self._invert:
            self.write(_INVON, None)


class ST7735S(ST7735):
    """A simple driver for the ST7735S-based displays."""

    _INIT = (
        # Frame Rate
        (_FRMCTR1, b"\x01\x2c\x2d"),
        (_FRMCTR2, b"\x01\x2c\x2d"),
        (_FRMCTR3, b"\x01\x2c\x2d\x01\x2c\x2d"),
        # Column inversion
        (_INVCTR, b"\x07"),
        # Power Sequence
        (_PWCTR1, b"\xa2\x02\x84"),
        (_PWCTR2, b"\xc5"),
        (_PWCTR3, b"\x0a\x00"),
        (_PWCTR4, b"\x8a\x2a"),
        (_PWCTR5, b"\x8a\xee"),
        # VCOM
        (_VMCTR1, b"\x0e"),
        # Gamma
        (
            _GMCTRP1,
            b"\x0f\x1a\x0f\x18\x2f\x28\x20\x22" b"\x1f\x1b\x23\x37\x00\x07\x02\x10",
        ),
        (
            _GMCTRN1,
            b"\x0f\x1b\x0f\x17\x33\x2c\x29\x2e" b"\x30\x30\x39\x3f\x00\x07\x03\x10",
        ),
        # 65k mode
        (_COLMOD, b"\x05"),
        # set scan direction: up to down, right to left
        (_MADCTL, b"\x60"),
        (_SLPOUT, None),
        (_DISPON, None),
    )

    # pylint: disable-msg=useless-super-delegation, too-many-arguments
    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        bl: digitalio.DigitalInOut,  # Backlight
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 128,
        height: int = 160,
        baudrate: int = 16000000,
        polarity: int = 0,
        phase: int = 0,
        *,
        x_offset: int = 2,
        y_offset: int = 1,
        rotation: int = 0,
    ) -> None:
        self._bl = bl
        # Turn on backlight
        self._bl.switch_to_output(value=1)
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
