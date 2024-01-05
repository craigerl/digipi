"""asyncio Protocol for extracting individual frames from CRLF-delimited TNC2."""
import logging
from typing import Iterable

from attrs import define, field

from .frame import Frame
from .decode import FrameDecodeProtocol, GenericDecoder

__author__ = "Masen Furer KF7HVM <kf7hvm@0x26.net>"
__copyright__ = "Copyright 2022 Masen Furer and Contributors"
__license__ = "Apache License, Version 2.0"


log = logging.getLogger(__name__)


@define
class TNC2Decode(GenericDecoder[Frame]):
    """
    Decode packets in CR-LF delimited TNC2 monitor format.

    Example: SENDER>DEST,PATH:data for the packet
    """

    @staticmethod
    def decode_frames(frame: bytes) -> Iterable[Frame]:
        try:
            yield Frame.from_str(frame.decode("latin1"))
        except Exception:
            log.debug("Ignore frame decode error %r", frame, exc_info=True)

    def update(self, new_data: bytes) -> Iterable[Frame]:
        packets = new_data.splitlines()
        for packet in packets:
            if packet.strip().startswith(b"#"):
                log.debug(packet)  # log comment
                continue
            if packet:
                yield from self.decode_frames(packet)


@define
class TNC2Protocol(FrameDecodeProtocol[Frame]):
    """Protocol for decoding a stream of TNC2 format packets."""

    decoder: TNC2Decode = field(factory=TNC2Decode)

    def write(self, frame: Frame) -> None:
        """Write the Frame to the transport."""
        return self.transport.write(str(frame).encode("latin1") + b"\r\n")
