# SPDX-FileCopyrightText: Copyright (c) 2022 Dan Halbert for Adafruit Industries
#
# SPDX-License-Identifier: MIT
"""
`circuitpython_typing`
================================================================================

Types needed for type annotation that are not in `typing`


* Author(s): Alec Delaney, Dan Halbert, Randy Hudson
"""

__version__ = "1.12.2"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_Typing.git"

import array
from typing import TYPE_CHECKING, Optional, Union

# Protocol was introduced in Python 3.8, TypeAlias in 3.10
from typing_extensions import Protocol, TypeAlias

# pylint: disable=used-before-assignment
if TYPE_CHECKING:
    import alarm
    import audiocore
    import audiomixer
    import audiomp3
    import rgbmatrix
    import synthio
    import ulab
    from alarm.pin import PinAlarm
    from alarm.time import TimeAlarm
    from ulab.numpy import ndarray


# Lists below are alphabetized.

# More added in each conditional import.
__all__ = [
    "Alarm",
    "AudioSample",
    "BlockDevice",
    "ByteStream",
    "FrameBuffer",
    "ReadableBuffer",
    "WriteableBuffer",
]

ReadableBuffer: TypeAlias = Union[
    array.array,
    bytearray,
    bytes,
    memoryview,
    "rgbmatrix.RGBMatrix",
    "ulab.numpy.ndarray",
]
"""Classes that implement the readable buffer protocol."""

WriteableBuffer: TypeAlias = Union[
    array.array,
    bytearray,
    memoryview,
    "rgbmatrix.RGBMatrix",
    "ulab.numpy.ndarray",
]
"""Classes that implement the writeable buffer protocol."""


class ByteStream(Protocol):
    """Protocol for basic I/O operations on a byte stream.
    Classes which implement this protocol include
    * `io.BytesIO`
    * `io.FileIO` (for a file open in binary mode)
    * `busio.UART`
    * `usb_cdc.Serial`
    """

    def read(self, count: Optional[int] = None, /) -> Optional[bytes]:
        """Read ``count`` bytes from the stream.
        If ``count`` bytes are not immediately available,
        or if the parameter is not specified in the call,
        the outcome is implementation-dependent.
        """

    def write(self, buf: ReadableBuffer, /) -> Optional[int]:
        """Write the bytes in ``buf`` to the stream."""


class BlockDevice(Protocol):
    """Protocol for block device objects to enable a device to support
    CircuitPython filesystems. Classes which implement this protocol
    include `storage.VfsFat`.
    """

    def readblocks(self, start_block: int, buf: WriteableBuffer) -> None:
        """Read aligned, multiples of blocks. Starting at
        the block given by the index ``start_block``, read blocks
        from the device into ``buf``. The number of blocks to
        read is given by the length of ``buf``, which will be a
        multiple of the block size.
        """

    def writeblocks(self, start_block: int, buf: ReadableBuffer) -> None:
        """Write aligned, multiples of blocks, and require that
        the blocks that are written to be first erased (if necessary)
        by this method. Starting at the block given by the index
        ``start_block``, write blocks from ``buf`` to the device.
        The number of blocks to write is given by the length
        of ``buf``, which will be a multiple of the block size.
        """


# These types may not be in adafruit-blinka, so use the string form instead of a resolved name.

AudioSample: TypeAlias = Union[
    "audiocore.WaveFile",
    "audiocore.RawSample",
    "audiomixer.Mixer",
    "audiomp3.MP3Decoder",
    "synthio.MidiTrack",
]
"""Classes that implement the audiosample protocol.
You can play these back with `audioio.AudioOut`, `audiobusio.I2SOut` or `audiopwmio.PWMAudioOut`.
"""

FrameBuffer: TypeAlias = Union["rgbmatrix.RGBMatrix"]
"""Classes that implement the framebuffer protocol."""

Alarm: TypeAlias = Union["alarm.pin.PinAlarm", "alarm.time.TimeAlarm"]
"""Classes that implement alarms for sleeping and asynchronous notification.
You can use these alarms to wake up from light or deep sleep.
"""
