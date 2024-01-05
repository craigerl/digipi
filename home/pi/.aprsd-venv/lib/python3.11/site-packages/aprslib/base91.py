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
Provides facilities for covertion from/to base91
"""

__all__ = ['to_decimal', 'from_decimal']
from math import log, ceil
import sys
from re import findall
from aprslib import string_type, int_type

if sys.version_info < (3,):
    _range = xrange
else:
    _range = range


def to_decimal(text):
    """
    Takes a base91 char string and returns decimal
    """

    if not isinstance(text, string_type):
        raise TypeError("expected str or unicode, %s given" % type(text))

    if findall(r"[\x00-\x20\x7c-\xff]", text):
        raise ValueError("invalid character in sequence")

    text = text.lstrip('!')
    decimal = 0
    length = len(text) - 1
    for i, char in enumerate(text):
        decimal += (ord(char) - 33) * (91 ** (length - i))

    return decimal if text != '' else 0


def from_decimal(number, width=1):
    """
    Takes a decimal and returns base91 char string.
    With optional parameter for fix with output
    """
    text = []

    if not isinstance(number, int_type):
        raise TypeError("Expected number to be int, got %s", type(number))
    elif not isinstance(width, int_type):
        raise TypeError("Expected width to be int, got %s", type(number))
    elif number < 0:
        raise ValueError("Expected number to be positive integer")
    elif number > 0:
        max_n = ceil(log(number) / log(91))

        for n in _range(int(max_n), -1, -1):
            quotient, number = divmod(number, 91**n)
            text.append(chr(33 + quotient))

    return "".join(text).lstrip('!').rjust(max(1, width), '!')
