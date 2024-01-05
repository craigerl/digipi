"""AX.25 frame encode/decode"""
import enum
import logging
from typing import Any, Iterable, Optional, Sequence, Union

from attrs import define, field
from attr import converters, validators
from bitarray import bitarray

from . import util
from .address import Address
from .decode import GenericDecoder
from .fcs import FCS


__author__ = "Masen Furer KF7HVM <kf7hvm@0x26.net>"
__copyright__ = "Copyright 2022 Masen Furer and Contributors"
__license__ = "Apache License, Version 2.0"

_logger = logging.getLogger(__name__)

AX25_FLAG = 0x7E
AX25_FLAG_B = bytes([AX25_FLAG])

# AX.25 Protocol ID — This field is set to 0xf0 (no layer 3 protocol).
NO_PROTOCOL_ID = b"\xF0"
# AX.25 Control Field — This field is set to 0x03 (UI-frame).
UI_CONTROL_FIELD = b"\x03"


class FrameType(enum.Enum):
    """
    Determines the type of the packet based on the control field.
    """

    U_TEST = 0xE3
    U_XID = 0xAF
    U_FRMR = 0x87
    U_SABME = 0x6F
    U_UA = 0x63
    U_DISC = 0x43
    U_SABM = 0x2F
    U_DM = 0xF
    S_SREJ = 0xD
    S_REJ = 0x9
    S_RNR = 0x5
    U_UI = 0x3
    S_RR = 0x1
    I = 0x0  # noqa: E741

    @classmethod
    def from_control_byte(cls, control: int):
        for val in cls.__members__.values():
            if control & val.value == val.value:
                return val
        raise ValueError(
            "Cannot interpret control byte {!r} as a valid AX.25 frame type."
        )


def bytes_from_int(b_or_i) -> bytes:
    if isinstance(b_or_i, int):
        return bytes([b_or_i])
    return bytes(b_or_i)


@define(frozen=True, slots=True)
class Control:
    """The Frame control byte, indicates FrameType and PID"""
    v: bytes = field(
        validator=util.valid_length(1, 1, validators.instance_of(bytes)),
        converter=bytes_from_int,
    )

    @property
    def bv(self) -> bitarray:
        bv = bitarray()
        bv.frombytes(self.v)
        return bv

    @property
    def ftype(self) -> FrameType:
        """The FrameType associated with this Control byte."""
        return FrameType.from_control_byte(self.v[0])

    @classmethod
    def from_any(cls, obj: Any) -> "Control":
        if isinstance(obj, cls):
            return obj
        return Control(obj)

    @property
    def n_r(self) -> int:
        return self.v[0] >> 5

    @property
    def n_s(self) -> int:
        return (self.v[0] & 0x0F) >> 1

    @property
    def p_f(self) -> bool:
        return bool(self.bv[4])

    def __bytes__(self) -> bytes:
        return self.v


def bytes_or_encode_utf8(v):
    if isinstance(v, (bytes, bytearray)):
        return bytes(v)
    return str(v).encode("utf-8")


@define(frozen=True, slots=True)
class Frame:
    """
    An AX.25 Frame.
    """

    # how large the `control` field is: 1 byte for UN frames
    CONTROL_SIZE = 1

    destination: Address
    source: Address
    path: Sequence[Address]
    control: Control = field(default=Control(UI_CONTROL_FIELD), converter=Control.from_any)
    pid: Optional[bytes] = field(
        default=NO_PROTOCOL_ID,
        validator=validators.optional(
            util.valid_length(1, 1, validators.instance_of(bytes)),
        ),
        converter=converters.optional(bytes_from_int),
    )
    info: bytes = field(default=b"", converter=bytes_or_encode_utf8)

    @classmethod
    def ui(
        cls,
        destination: Union[Address, str],
        source: Union[Address, str],
        path: Optional[Sequence[Union[Address, str]]] = None,
        info: bytes = b"",
    ):
        """Create a UI frame with the given information."""
        return cls(
            destination=Address.from_any(destination),
            source=Address.from_any(source, a7_hldc=not bool(path)),
            path=[Address.from_any(p, a7_hldc=(p == path[-1])) for p in path or []],
            info=info,
        )

    @classmethod
    def from_bytes(cls, ax25_bytes: bytes) -> "Frame":
        """
        Decode the frame from AX.25 bytes.
        """
        destination = Address.from_bytes(ax25_bytes[:7])
        source = last_address = Address.from_bytes(ax25_bytes[7:14])
        path = []
        path_start = 14
        while not last_address.a7_hldc:
            last_address = Address.from_bytes(ax25_bytes[path_start : path_start + 7])
            path.append(last_address)
            path_start += 7
        info_start = control_end = path_start + cls.CONTROL_SIZE
        control = Control(ax25_bytes[path_start:control_end])
        if control.ftype in (FrameType.I, FrameType.U_UI):
            info_start += 1
            pid = ax25_bytes[control_end:info_start]
        else:
            pid = None
        return cls(
            destination=destination,
            source=source,
            path=path,
            control=control,
            pid=pid,
            info=ax25_bytes[info_start:],
        )

    def __bytes__(self) -> bytes:
        """Encode the frame as AX.25."""
        encoded_frame = [
            bytes(self.destination),
            bytes(self.source),
            *(bytes(p) for p in self.path),
            bytes(self.control),
        ]
        if self.control.ftype in (FrameType.I, FrameType.U_UI):
            encoded_frame.append(self.pid)
        encoded_frame.append(bytes(self.info))
        return b"".join(encoded_frame)

    @classmethod
    def from_str(cls, ax25_text: str) -> "Frame":
        """Decode the frame from TNC2 monitor format."""
        source_text, gt, rem = ax25_text.partition(">")
        address_field, colon, info_text = rem.partition(":")
        destination_text, *paths_text = address_field.split(",")
        return cls.ui(
            destination=destination_text,
            source=source_text,
            path=paths_text,
            info=info_text.encode("latin1"),
        )

    def __str__(self) -> str:
        """Serialize the frame as TNC2 monitor format."""
        full_path = [
            str(self.destination),
            *(str(p) for p in self.path or []),
        ]
        return "%s>%s:%s" % (
            str(self.source),
            ",".join(full_path),
            bytes(self.info).decode("latin1"),
        )


@define
class AX25BytestreamDecoder(GenericDecoder[Frame]):
    """Decode a generic AX25_FLAG delimited bytestream"""

    _last_pframe: bytes = field(default=b"", init=False)

    @staticmethod
    def decode_frames(frame: bytes) -> Iterable[Frame]:
        """
        Decode a single deframed byte chunk.

        :param frame: should represent a single higher level frame to
            decode in some way.
        """
        if frame:
            yield Frame.from_bytes(frame)

    def update(self, new_data: bytes) -> Iterable[Frame]:
        """
        Decode the next sequence of bytes from the stream.

        :param new_data: the next bytes from the stream
        :return: an iterable of decoded frames
        """
        packet_start = 0
        data, self._last_pframe = self._last_pframe + new_data, b""
        while 0 < (packet_start + 1) < len(data) or len(data) == 1:
            if data[packet_start] != AX25_FLAG:
                _logger.debug(
                    "AX.25 frame did not start with flag {}, got {} "
                    "instead (treating it as the start)".format(
                        bin(AX25_FLAG),
                        bin(data[packet_start]),
                    )
                )
            while (packet_start + 1) < len(data) and data[packet_start] == AX25_FLAG:
                # consume flag bytes until data is reached
                packet_start += 1
            # find the end of the packet
            end_flag_at = data.find(AX25_FLAG, packet_start)
            if end_flag_at < 0:
                # didn't find the end, so add to the buffer and continue
                self._last_pframe = data[packet_start:]
                break
            else:
                fcs = data[end_flag_at - 2 : end_flag_at] or None
                for frame in self.decode_frames(data[packet_start : end_flag_at - 2]):
                    calc_fcs = FCS.from_bytes(bytes(frame)).digest()
                    if calc_fcs != fcs:
                        raise ValueError(
                            "FCS did not match for {}: {!r} != {!r}".format(
                                frame, calc_fcs, fcs
                            )
                        )
                    yield frame
                packet_start = end_flag_at

    def flush(self) -> Iterable[Frame]:
        """Call when the stream is closing to decode any final buffered bytes."""
        if self._last_pframe:
            yield from self.decode_frames(self._last_pframe)
        return
