# SPDX-FileCopyrightText: 2017 Radomir Dopieralski for Adafruit Industries
# SPDX-FileCopyrightText: 2023 Matt Land
#
# SPDX-License-Identifier: MIT

"""
`adafruit_rgb_display.rgb`
====================================================

Base class for all RGB Display devices

* Author(s): Radomir Dopieralski, Michael McWethy, Matt Land
"""

import struct
import time

try:
    from typing import Optional, Union, Tuple, List, Any, ByteString
    import digitalio
    import busio

    from circuitpython_typing.pil import Image
except ImportError:
    pass

try:
    import numpy
except ImportError:
    numpy = None

from adafruit_bus_device import spi_device

__version__ = "3.12.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_RGB_Display.git"

# This is the size of the buffer to be used for fill operations, in 16-bit
# units.
_BUFFER_SIZE = 256
try:
    import platform

    if "CPython" in platform.python_implementation():
        _BUFFER_SIZE = 320 * 240  # blit the whole thing
except ImportError:
    pass


def color565(
    r: Union[int, Tuple[int, int, int], List[int]],
    g: int = 0,
    b: int = 0,
) -> int:
    """Convert red, green and blue values (0-255) into a 16-bit 565 encoding.  As
    a convenience this is also available in the parent adafruit_rgb_display
    package namespace."""
    if not isinstance(r, int):  # see if the first var is a tuple/list
        red, g, b = r
    else:
        red = r
    return (red & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3


def image_to_data(image: Image) -> Any:
    """Generator function to convert a PIL image to 16-bit 565 RGB bytes."""
    # NumPy is much faster at doing this. NumPy code provided by:
    # Keith (https://www.blogger.com/profile/02555547344016007163)
    data = numpy.array(image.convert("RGB")).astype("uint16")
    color = (
        ((data[:, :, 0] & 0xF8) << 8)
        | ((data[:, :, 1] & 0xFC) << 3)
        | (data[:, :, 2] >> 3)
    )
    return numpy.dstack(((color >> 8) & 0xFF, color & 0xFF)).flatten().tolist()


class DummyPin:
    """Can be used in place of a ``DigitalInOut()`` when you don't want to skip it."""

    def deinit(self) -> None:
        """Dummy DigitalInOut deinit"""

    def switch_to_output(
        self, *, value: bool = False, drive_mode: Optional[digitalio.DriveMode] = None
    ) -> None:
        """Dummy switch_to_output method"""

    def switch_to_input(self, *, pull: Optional[digitalio.Pull] = None) -> None:
        """Dummy switch_to_input method"""

    @property
    def value(self) -> bool:
        """Dummy value DigitalInOut property"""

    @value.setter
    def value(self, val: digitalio.DigitalInOut) -> None:
        pass

    @property
    def direction(self) -> digitalio.Direction:
        """Dummy direction DigitalInOut property"""

    @direction.setter
    def direction(self, val: digitalio.Direction) -> None:
        pass

    @property
    def pull(self) -> digitalio.Pull:
        """Dummy pull DigitalInOut property"""

    @pull.setter
    def pull(self, val: digitalio.Pull) -> None:
        pass


class Display:  # pylint: disable-msg=no-member
    """Base class for all RGB display devices
    :param width: number of pixels wide
    :param height: number of pixels high
    """

    _PAGE_SET: Optional[int] = None
    _COLUMN_SET: Optional[int] = None
    _RAM_WRITE: Optional[int] = None
    _RAM_READ: Optional[int] = None
    _X_START = 0  # pylint: disable=invalid-name
    _Y_START = 0  # pylint: disable=invalid-name
    _INIT: Tuple[Tuple[int, Union[ByteString, None]], ...] = ()
    _ENCODE_PIXEL = ">H"
    _ENCODE_POS = ">HH"
    _DECODE_PIXEL = ">BBB"

    def __init__(self, width: int, height: int, rotation: int) -> None:
        self.width = width
        self.height = height
        if rotation not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        self._rotation = rotation
        self.init()

    def write(
        self, command: Optional[int] = None, data: Optional[ByteString] = None
    ) -> None:
        """Abstract method"""
        raise NotImplementedError()

    def read(self, command: Optional[int] = None, count: int = 0) -> ByteString:
        """Abstract method"""
        raise NotImplementedError()

    def init(self) -> None:
        """Run the initialization commands."""
        for command, data in self._INIT:
            self.write(command, data)

    # pylint: disable-msg=invalid-name,too-many-arguments
    def _block(
        self, x0: int, y0: int, x1: int, y1: int, data: Optional[ByteString] = None
    ) -> Optional[ByteString]:
        """Read or write a block of data."""
        self.write(
            self._COLUMN_SET, self._encode_pos(x0 + self._X_START, x1 + self._X_START)
        )
        self.write(
            self._PAGE_SET, self._encode_pos(y0 + self._Y_START, y1 + self._Y_START)
        )
        if data is None:
            size = struct.calcsize(self._DECODE_PIXEL)
            return self.read(self._RAM_READ, (x1 - x0 + 1) * (y1 - y0 + 1) * size)
        self.write(self._RAM_WRITE, data)
        return None

    # pylint: enable-msg=invalid-name,too-many-arguments

    def _encode_pos(self, x: int, y: int) -> bytes:
        """Encode a position into bytes."""
        return struct.pack(self._ENCODE_POS, x, y)

    def _encode_pixel(self, color: Union[int, Tuple]) -> bytes:
        """Encode a pixel color into bytes."""
        return struct.pack(self._ENCODE_PIXEL, color)

    def _decode_pixel(self, data: Union[bytes, Union[bytearray, memoryview]]) -> int:
        """Decode bytes into a pixel color."""
        return color565(*struct.unpack(self._DECODE_PIXEL, data))

    def pixel(
        self, x: int, y: int, color: Optional[Union[int, Tuple]] = None
    ) -> Optional[int]:
        """Read or write a pixel at a given position."""
        if color is None:
            return self._decode_pixel(self._block(x, y, x, y))  # type: ignore[arg-type]

        if 0 <= x < self.width and 0 <= y < self.height:
            self._block(x, y, x, y, self._encode_pixel(color))
        return None

    def image(
        self,
        img: Image,
        rotation: Optional[int] = None,
        x: int = 0,
        y: int = 0,
    ) -> None:
        """Set buffer to value of Python Imaging Library image. The image should
        be in 1 bit mode and a size not exceeding the display size when drawn at
        the supplied origin."""
        if rotation is None:
            rotation = self.rotation
        if not img.mode in ("RGB", "RGBA"):
            raise ValueError("Image must be in mode RGB or RGBA")
        if rotation not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        if rotation != 0:
            img = img.rotate(rotation, expand=True)
        imwidth, imheight = img.size
        if x + imwidth > self.width or y + imheight > self.height:
            raise ValueError(
                "Image must not exceed dimensions of display ({0}x{1}).".format(
                    self.width, self.height
                )
            )
        if numpy:
            pixels = bytes(image_to_data(img))
        else:
            # Slower but doesn't require numpy
            pixels = bytearray(imwidth * imheight * 2)
            for i in range(imwidth):
                for j in range(imheight):
                    pix = color565(img.getpixel((i, j)))
                    pixels[2 * (j * imwidth + i)] = pix >> 8
                    pixels[2 * (j * imwidth + i) + 1] = pix & 0xFF
        self._block(x, y, x + imwidth - 1, y + imheight - 1, pixels)

    # pylint: disable-msg=too-many-arguments
    def fill_rectangle(
        self, x: int, y: int, width: int, height: int, color: Union[int, Tuple]
    ) -> None:
        """Draw a rectangle at specified position with specified width and
        height, and fill it with the specified color."""
        x = min(self.width - 1, max(0, x))
        y = min(self.height - 1, max(0, y))
        width = min(self.width - x, max(1, width))
        height = min(self.height - y, max(1, height))
        self._block(x, y, x + width - 1, y + height - 1, b"")
        chunks, rest = divmod(width * height, _BUFFER_SIZE)
        pixel = self._encode_pixel(color)
        if chunks:
            data = pixel * _BUFFER_SIZE
            for _ in range(chunks):
                self.write(None, data)
        self.write(None, pixel * rest)

    # pylint: enable-msg=too-many-arguments

    def fill(self, color: Union[int, Tuple] = 0) -> None:
        """Fill the whole display with the specified color."""
        self.fill_rectangle(0, 0, self.width, self.height, color)

    def hline(self, x: int, y: int, width: int, color: Union[int, Tuple]) -> None:
        """Draw a horizontal line."""
        self.fill_rectangle(x, y, width, 1, color)

    def vline(self, x: int, y: int, height: int, color: Union[int, Tuple]) -> None:
        """Draw a vertical line."""
        self.fill_rectangle(x, y, 1, height, color)

    @property
    def rotation(self) -> int:
        """Set the default rotation"""
        return self._rotation

    @rotation.setter
    def rotation(self, val: int) -> None:
        if val not in (0, 90, 180, 270):
            raise ValueError("Rotation must be 0/90/180/270")
        self._rotation = val


class DisplaySPI(Display):
    """Base class for SPI type devices"""

    # pylint: disable-msg=too-many-arguments
    def __init__(
        self,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        cs: digitalio.DigitalInOut,
        rst: Optional[digitalio.DigitalInOut] = None,
        width: int = 1,
        height: int = 1,
        baudrate: int = 12000000,
        polarity: int = 0,
        phase: int = 0,
        *,
        x_offset: int = 0,
        y_offset: int = 0,
        rotation: int = 0
    ):
        self.spi_device = spi_device.SPIDevice(
            spi, cs, baudrate=baudrate, polarity=polarity, phase=phase
        )
        self.dc_pin = dc
        self.rst = rst
        self.dc_pin.switch_to_output(value=0)
        if self.rst:
            self.rst.switch_to_output(value=0)
            self.reset()
        self._X_START = x_offset  # pylint: disable=invalid-name
        self._Y_START = y_offset  # pylint: disable=invalid-name
        super().__init__(width, height, rotation)

    # pylint: enable-msg=too-many-arguments

    def reset(self) -> None:
        """Reset the device"""
        if not self.rst:
            raise RuntimeError("a reset pin was not provided")
        self.rst.value = 0
        time.sleep(0.050)  # 50 milliseconds
        self.rst.value = 1
        time.sleep(0.050)  # 50 milliseconds

    # pylint: disable=no-member
    def write(
        self, command: Optional[int] = None, data: Optional[ByteString] = None
    ) -> None:
        """SPI write to the device: commands and data"""
        if command is not None:
            self.dc_pin.value = 0
            with self.spi_device as spi:
                spi.write(bytearray([command]))
        if data is not None:
            self.dc_pin.value = 1
            with self.spi_device as spi:
                spi.write(data)

    def read(self, command: Optional[int] = None, count: int = 0) -> ByteString:
        """SPI read from device with optional command"""
        data = bytearray(count)
        self.dc_pin.value = 0
        with self.spi_device as spi:
            if command is not None:
                spi.write(bytearray([command]))
            if count:
                spi.readinto(data)
        return data
