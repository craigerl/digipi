# SPDX-FileCopyrightText: Copyright (c) 2025 Tim Cocks for Adafruit Industries
#
# SPDX-License-Identifier: MIT

"""
`circuitpython_typing.displayio`
================================================================================

Type annotation definitions used for displayio related components.

* Author(s): Tim Cocks
"""

from typing import TypeAlias, Union

from busdisplay import BusDisplay
from epaperdisplay import EPaperDisplay

AnyDisplay: TypeAlias = Union[BusDisplay, EPaperDisplay, "framebufferio.FramebufferDisplay"]
