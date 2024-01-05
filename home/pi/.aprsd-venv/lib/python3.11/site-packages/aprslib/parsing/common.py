import re
from math import sqrt
from datetime import datetime
from aprslib import base91
from aprslib.exceptions import ParseError
from aprslib.parsing import logger
from aprslib.parsing.telemetry import parse_comment_telemetry

__all__ = [
    'validate_callsign',
    'parse_header',
    'parse_timestamp',
    'parse_comment',
    'parse_data_extentions',
    'parse_comment_altitude',
    'parse_dao',
    ]

def validate_callsign(callsign, prefix=""):
    prefix = '%s: ' % prefix if bool(prefix) else ''

    match = re.findall(r"^([A-Z0-9]{1,6})(-(\d{1,2}))?$", callsign)

    if not match:
        raise ParseError("%sinvalid callsign" % prefix)

    callsign, _, ssid = match[0]

    if bool(ssid) and int(ssid) > 15:
        raise ParseError("%sssid not in 0-15 range" % prefix)


def parse_header(head):
    """
    Parses the header part of packet
    Returns a dict
    """
    try:
        (fromcall, path) = head.split('>', 1)
    except:
        raise ParseError("invalid packet header")

    if (not 1 <= len(fromcall) <= 9 or
       not re.findall(r"^[a-z0-9]{0,9}(\-[a-z0-9]{1,8})?$", fromcall, re.I)):

        raise ParseError("fromcallsign is invalid")

    path = path.split(',')

    if len(path[0]) == 0:
        raise ParseError("no tocallsign in header")

    tocall = path[0]
    path = path[1:]

    validate_callsign(tocall, "tocallsign")

    for digi in path:
        if not re.findall(r"^[A-Z0-9\-]{1,9}\*?$", digi, re.I):
            raise ParseError("invalid callsign in path")

    parsed = {
        'from': fromcall,
        'to': tocall,
        'path': path,
        }

    viacall = ""
    if len(path) >= 2 and re.match(r"^q..$", path[-2]):
        viacall = path[-1]

    parsed.update({'via': viacall})

    return parsed


def parse_timestamp(body, packet_type=''):
    parsed = {}

    match = re.findall(r"^((\d{6})(.))$", body[0:7])
    if match:
        rawts, ts, form = match[0]
        utc = datetime.utcnow()

        timestamp = 0

        if packet_type == '>' and form != 'z':
            pass
        else:
            body = body[7:]

            try:
                # zulu hhmmss format
                if form == 'h':
                    timestamp = "%d%02d%02d%s" % (utc.year, utc.month, utc.day, ts)
                # zulu ddhhmm format
                # '/' local ddhhmm format
                elif form in 'z/':
                    timestamp = "%d%02d%s%02d" % (utc.year, utc.month, ts, 0)
                else:
                    timestamp = "19700101000000"

                td = utc.strptime(timestamp, "%Y%m%d%H%M%S") - datetime(1970, 1, 1)
                timestamp = int((td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6) / 10**6)
            except Exception as exp:
                timestamp = 0
                logger.debug(exp)

        parsed.update({
            'raw_timestamp': rawts,
            'timestamp': int(timestamp),
            })

    return (body, parsed)


def parse_comment(body, parsed):
    body, result = parse_data_extentions(body)
    parsed.update(result)

    body, result = parse_comment_altitude(body)
    parsed.update(result)

    body, result = parse_comment_telemetry(body)
    parsed.update(result)

    body = parse_dao(body, parsed)

    if len(body) > 0 and body[0] == "/":
        body = body[1:]

    parsed.update({'comment': body.strip(' ')})


def parse_data_extentions(body):
    parsed = {}

    # course speed bearing nrq
    # Page 27 of the spec
    # format: 111/222/333/444text
    match = re.findall(r"^([0-9 \.]{3})/([0-9 \.]{3})", body)
    if match:
        cse, spd = match[0]
        body = body[7:]
        if cse.isdigit() and cse != "000":
            parsed.update({'course': int(cse) if 1 <= int(cse) <= 360 else 0})
        if spd.isdigit() and spd != "000":
            parsed.update({'speed': int(spd)*1.852})

        # DF Report format
        # Page 29 of teh spec
        match = re.findall(r"^/([0-9 \.]{3})/([0-9 \.]{3})", body)
        if match:
            # cse=000 means stations is fixed, Page 29 of the spec
            if cse == '000':
                parsed.update({'course': 0})
            brg, nrq = match[0]
            body = body[8:]
            if brg.isdigit():
                parsed.update({'bearing': int(brg)})
            if nrq.isdigit():
                parsed.update({'nrq': int(nrq)})
    else:
        # PHG format: PHGabcd....
        # RHGR format: RHGabcdr/....
        match = re.findall(r"^(PHG(\d[\x30-\x7e]\d\d)([0-9A-Z]\/)?)", body)
        if match:
            ext, phg, phgr = match[0]
            body = body[len(ext):]
            parsed.update({
                'phg': phg,
                'phg_power': int(phg[0]) ** 2, # watts
                'phg_height': (10 * (2 ** (ord(phg[1]) - 0x30))) * 0.3048, # in meters
                'phg_gain': 10 ** (int(phg[2]) / 10.0), # dB
                })

            phg_dir = int(phg[3])
            if phg_dir == 0:
                phg_dir = 'omni'
            elif phg_dir == 9:
                phg_dir = 'invalid'
            else:
                phg_dir = 45 * phg_dir

            parsed['phg_dir'] = phg_dir
            # range in km
            parsed['phg_range'] = sqrt(2 * (parsed['phg_height'] / 0.3048)
                                       * sqrt((parsed['phg_power'] / 10.0)
                                               * (parsed['phg_gain'] / 2.0)
                                              )
                                       ) * 1.60934

            if phgr:
                # PHG rate per hour
                parsed['phg'] += phgr[0]
                parsed.update({'phg_rate': int(phgr[0], 16)}) # as decimal
        else:
                match = re.findall(r"^RNG(\d{4})", body)
                if match:
                    rng = match[0]
                    body = body[7:]
                    parsed.update({'rng': int(rng) * 1.609344})  # miles to km

    return body, parsed

def parse_comment_altitude(body):
    parsed = {}
    match = re.findall(r"^(.*?)/A=(\-\d{5}|\d{6})(.*)$", body)
    if match:
        body, altitude, rest = match[0]
        body += rest
        parsed.update({'altitude': int(altitude)*0.3048})

    return body, parsed


def parse_dao(body, parsed):
    match = re.findall("^(.*)\!([\x21-\x7b])([\x20-\x7b]{2})\!(.*?)$", body)
    if match:
        body, daobyte, dao, rest = match[0]
        body += rest

        parsed.update({'daodatumbyte': daobyte.upper()})
        lat_offset = lon_offset = 0

        if daobyte == 'W' and dao.isdigit():
            lat_offset = int(dao[0]) * 0.001 / 60
            lon_offset = int(dao[1]) * 0.001 / 60
        elif daobyte == 'w' and ' ' not in dao:
            lat_offset = (base91.to_decimal(dao[0]) / 91.0) * 0.01 / 60
            lon_offset = (base91.to_decimal(dao[1]) / 91.0) * 0.01 / 60

        parsed['latitude'] += lat_offset if parsed['latitude'] >= 0 else -lat_offset
        parsed['longitude'] += lon_offset if parsed['longitude'] >= 0 else -lon_offset

    return body
