import re
from aprslib.exceptions import ParseError
from aprslib.parsing.common import parse_timestamp

__all__ = [
        'parse_status',
        'parse_invalid',
        'parse_user_defined',
        ]


# STATUS PACKET
#
# >DDHHMMzComments
# >Comments
def parse_status(packet_type, body):
    body, result = parse_timestamp(body, packet_type)

    result.update({
        'format': 'status',
        'status': body.strip(' ')
        })

    return (body, result)


# INVALID
#
# ,.........................
def parse_invalid(body):
    return ('', {
        'format': 'invalid',
        'body': body
        })


# USER DEFINED
#
# {A1................
# {{.................
def parse_user_defined(body):
    return ('', {
        'format': 'user-defined',
        'id': body[0],
        'type': body[1],
        'body': body[2:],
        })
