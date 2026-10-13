# aprslib - Python library for working with APRS
# Copyright (C) 2013-2014 Rossen Georgiev
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""
Contains a function for generating passcode from callsign
"""


def passcode(callsign):
    """
    Takes a CALLSIGN and returns passcode
    """
    assert isinstance(callsign, str)

    callsign = callsign.split('-')[0].upper()

    code = 0x73e2
    for i, char in enumerate(callsign):
        code ^= ord(char) << (8 if not i % 2 else 0)

    return code & 0x7fff
