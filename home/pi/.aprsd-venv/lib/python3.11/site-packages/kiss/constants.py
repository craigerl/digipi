"""Python KISS Module Constants."""

import logging
import os

__author__ = "Greg Albrecht W2GMD <oss@undef.net>"
__copyright__ = "Copyright 2017 Greg Albrecht and Contributors"
__license__ = "Apache License, Version 2.0"


if bool(os.environ.get("DEBUG")):
    LOG_LEVEL = logging.DEBUG
    logging.debug("Debugging Enabled via DEBUG Environment Variable.")
else:
    LOG_LEVEL = logging.INFO

LOG_FORMAT = logging.Formatter(
    "%(asctime)s kiss %(levelname)s %(name)s.%(funcName)s:%(lineno)d - %(message)s",
)

SERIAL_TIMEOUT = 0.01
READ_BYTES = 1000

# KISS Special Characters
# http://en.wikipedia.org/wiki/KISS_(TNC)#Special_Characters
# http://k4kpk.com/content/notes-aprs-kiss-and-setting-tnc-x-igate-and-digipeater
# Frames begin and end with a FEND/Frame End/0xC0 byte
FEND = b"\xC0"  # Marks START and END of a Frame
FESC = b"\xDB"  # Escapes FEND and FESC bytes within a frame

# Transpose Bytes: Used within a frame-
# "Transpose FEND": An FEND after an FESC (within a frame)-
# Sent as FESC TFEND
TFEND = b"\xDC"
# "Transpose FESC": An FESC after an FESC (within a frame)-
# Sent as FESC TFESC
TFESC = b"\xDD"

# "FEND is sent as FESC, TFEND"
# 0xC0 is sent as 0xDB 0xDC
FESC_TFEND = b"".join([FESC, TFEND])

# "FESC is sent as FESC, TFESC"
# 0xDB is sent as 0xDB 0xDD
FESC_TFESC = b"".join([FESC, TFESC])

# KISS Command Codes
# http://en.wikipedia.org/wiki/KISS_(TNC)#Command_Codes
DATA_FRAME = b"\x00"
TX_DELAY = b"\x01"
PERSISTENCE = b"\x02"
SLOT_TIME = b"\x03"
TX_TAIL = b"\x04"
FULL_DUPLEX = b"\x05"
SET_HARDWARE = b"\x06"
RETURN = b"\xFF"

# Alternate convenience spellings for Command Codes
# (these more closely match the names in the spec)
DATAFRAME = DATA_FRAME
TXDELAY = TX_DELAY
P = PERSISTENCE
SLOTTIME = SLOT_TIME
TXTAIL = TX_TAIL
FULLDUPLEX = FULL_DUPLEX
SETHARDWARE = SET_HARDWARE

DEFAULT_KISS_CONFIG_VALUES = {
    "TX_DELAY": 40,
    "PERSISTENCE": 63,
    "SLOT_TIME": 20,
    "TX_TAIL": 30,
    "FULL_DUPLEX": 0,
}

KISS_ON = "KISS $0B"
KISS_OFF = b"".join([FEND, RETURN, FEND, FEND])

NMEA_HEADER = b"".join([FEND, b"\xF0", b"$"])

# AX.25 Protocol ID — This field is set to 0xf0 (no layer 3 protocol).
NO_PROTOCOL_ID = b"\xF0"
# AX.25 Control Field — This field is set to 0x03 (UI-frame).
UI_CONTROL_FIELD = b"\x03"
