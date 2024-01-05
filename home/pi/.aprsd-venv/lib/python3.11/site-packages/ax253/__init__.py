from importlib_metadata import version

from .address import Address
from .decode import GenericDecoder, FrameDecodeProtocol, SyncFrameDecode
from .frame import AX25BytestreamDecoder, Control, Frame, FrameType
from .tnc2 import TNC2Protocol, TNC2Decode

__author__ = "Masen Furer KF7HVM <kf7hvm@0x26.net>"
__copyright__ = "Copyright 2022 Masen Furer and Contributors"
__license__ = "Apache License, Version 2.0"
__distribution__ = "ax253"
__version__ = version(__distribution__)
__all__ = [
    "Address",
    "AX25BytestreamDecoder",
    "Control",
    "Frame",
    "FrameDecodeProtocol",
    "FrameType",
    "GenericDecoder",
    "SyncFrameDecode",
    "TNC2Decode",
    "TNC2Protocol",
    "__distribution__",
    "__version__",
]
