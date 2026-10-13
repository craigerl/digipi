import re
import math
from aprslib import base91
from aprslib.exceptions import ParseError
from aprslib.parsing.common import parse_dao
from aprslib.parsing.telemetry import parse_comment_telemetry

__all__ = [
        'parse_mice',
        ]

# Mic-e message type table

MTYPE_TABLE_STD = {
    "111": "M0: Off Duty",
    "110": "M1: En Route",
    "101": "M2: In Service",
    "100": "M3: Returning",
    "011": "M4: Committed",
    "010": "M5: Special",
    "001": "M6: Priority",
    "000": "Emergency",
    }
MTYPE_TABLE_CUSTOM = {
    "111": "C0: Custom-0",
    "110": "C1: Custom-1",
    "101": "C2: Custom-2",
    "100": "C3: Custom-3",
    "011": "C4: Custom-4",
    "010": "C5: Custom-5",
    "001": "C6: Custom-6",
    "000": "Emergency",
    }

# Mic-encoded packet
#
# 'lllc/s$/.........         Mic-E no message capability
# 'lllc/s$/>........         Mic-E message capability
# `lllc/s$/>........         Mic-E old posit
def parse_mice(dstcall, body):
    parsed = {'format': 'mic-e'}

    dstcall = dstcall.split('-')[0]

    # verify mic-e format
    if len(dstcall) != 6:
        raise ParseError("dstcall has to be 6 characters")
    if len(body) < 8:
        raise ParseError("packet data field is too short")
    if not re.match(r"^[0-9A-Z]{3}[0-9L-Z]{3}$", dstcall):
        raise ParseError("invalid dstcall")
    if not re.match(r"^[&-\x7f][&-a][\x1c-\x7f]{2}[\x1c-\x7d]"
                    r"[\x1c-\x7f][\x21-\x7e][\/\\0-9A-Z]", body):
        raise ParseError("invalid data format")

    # get symbol table and symbol
    parsed.update({
        'symbol': body[6],
        'symbol_table': body[7]
        })

    # parse latitude
    # the routine translates each characters into a lat digit as described in
    # 'Mic-E Destination Address Field Encoding' table
    tmpdstcall = ""
    for i in dstcall:
        if i in "KLZ":  # spaces
            tmpdstcall += " "
        elif ord(i) > 76:  # P-Y
            tmpdstcall += chr(ord(i) - 32)
        elif ord(i) > 57:  # A-J
            tmpdstcall += chr(ord(i) - 17)
        else:  # 0-9
            tmpdstcall += i

    # determine position ambiguity
    match = re.findall(r"^\d+( *)$", tmpdstcall)
    if not match:
        raise ParseError("invalid latitude ambiguity")

    posambiguity = len(match[0])
    parsed.update({
        'posambiguity': posambiguity
        })

    # adjust the coordinates be in center of ambiguity box
    tmpdstcall = list(tmpdstcall)
    if posambiguity > 0:
        if posambiguity >= 4:
            tmpdstcall[2] = '3'
        else:
            tmpdstcall[6 - posambiguity] = '5'

    tmpdstcall = "".join(tmpdstcall)

    latminutes = float(("%s.%s" % (tmpdstcall[2:4], tmpdstcall[4:6])).replace(" ", "0"))
    latitude = int(tmpdstcall[0:2]) + (latminutes / 60.0)

    # determine the sign N/S
    latitude = -latitude if ord(dstcall[3]) <= 0x4c else latitude

    parsed.update({
        'latitude': latitude
        })

    # parse message bits

    mbits = re.sub(r"[0-9L]", "0", dstcall[0:3])
    mbits = re.sub(r"[P-Z]", "1", mbits)
    mbits = re.sub(r"[A-K]", "2", mbits)

    parsed.update({
        'mbits': mbits
        })

    # resolve message type

    if mbits.find("2") > -1:
        parsed.update({
            'mtype': MTYPE_TABLE_CUSTOM[mbits.replace("2", "1")]
            })
    else:
        parsed.update({
            'mtype': MTYPE_TABLE_STD[mbits]
            })

    # parse longitude

    longitude = ord(body[0]) - 28  # decimal part of longitude
    longitude += 100 if ord(dstcall[4]) >= 0x50 else 0  # apply lng offset
    longitude += -80 if longitude >= 180 and longitude <= 189 else 0
    longitude += -190 if longitude >= 190 and longitude <= 199 else 0

    # long minutes
    lngminutes = ord(body[1]) - 28.0
    lngminutes += -60 if lngminutes >= 60 else 0

    # + (long hundredths of minutes)
    lngminutes += ((ord(body[2]) - 28.0) / 100.0)

    # apply position ambiguity
    # routines adjust longitude to center of the ambiguity box
    if posambiguity == 4:
        lngminutes = 30
    elif posambiguity == 3:
        lngminutes = (math.floor(lngminutes/10) + 0.5) * 10
    elif posambiguity == 2:
        lngminutes = math.floor(lngminutes) + 0.5
    elif posambiguity == 1:
        lngminutes = (math.floor(lngminutes*10) + 0.5) / 10.0
    elif posambiguity != 0:
        raise ParseError("Unsupported position ambiguity: %d" % posambiguity)

    longitude += lngminutes / 60.0

    # apply E/W sign
    longitude = 0 - longitude if ord(dstcall[5]) >= 0x50 else longitude

    parsed.update({
        'longitude': longitude
        })

    # parse speed and course
    speed = (ord(body[3]) - 28) * 10
    course = ord(body[4]) - 28
    quotient = int(course / 10.0)
    course += -(quotient * 10)
    course = course*100 + ord(body[5]) - 28
    speed += quotient

    speed += -800 if speed >= 800 else 0
    course += -400 if course >= 400 else 0

    speed *= 1.852  # knots * 1.852 = kmph
    parsed.update({
        'speed': speed,
        'course': course
        })

    # the rest of the packet can contain telemetry and comment

    if len(body) > 8:
        body = body[8:]

        # check for optional 2 or 5 channel telemetry
        match = re.findall(r"^('[0-9a-f]{10}|`[0-9a-f]{4})(.*)$", body)
        if match:
            hexdata, body = match[0]

            hexdata = hexdata[1:]             # remove telemtry flag
            channels = int(len(hexdata) / 2)  # determine number of channels
            hexdata = int(hexdata, 16)        # convert hex to int

            telemetry = []
            for i in range(channels):
                telemetry.insert(0, int(hexdata >> 8*i & 255))

            parsed.update({'telemetry': telemetry})

        # check for optional altitude
        match = re.findall(r"^(.*)([!-{]{3})\}(.*)$", body)
        if match:
            body, altitude, extra = match[0]

            altitude = base91.to_decimal(altitude) - 10000
            parsed.update({'altitude': altitude})

            body = body + extra

        # attempt to parse comment telemetry
        body, telemetry = parse_comment_telemetry(body)
        parsed.update(telemetry)

        # parse DAO extention
        body = parse_dao(body, parsed)

        # rest is a comment
        parsed.update({'comment': body.strip(' ')})

    return ('', parsed)
