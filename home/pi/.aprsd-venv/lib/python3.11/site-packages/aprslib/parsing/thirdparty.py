import re
from aprslib.parsing.__init__ import parse
from aprslib.exceptions import UnknownFormat
from aprslib.exceptions import ParseError

__all__ = [
        'parse_thirdparty',
        ]

def parse_thirdparty(body):
    parsed = {'format':'thirdparty'}

    # Parse sub-packet
    try:
        subpacket = parse(body)
    except (UnknownFormat,ParseError) as ukf:
        raise

    parsed.update({'subpacket':subpacket})

    return('',parsed)
