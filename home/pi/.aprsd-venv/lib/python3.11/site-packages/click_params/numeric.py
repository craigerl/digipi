"""Numeric parameter types"""
from decimal import Decimal, DecimalException
from fractions import Fraction

import click

from .base import BaseParamType, ListParamType, RangeParamType


class DecimalParamType(BaseParamType):
    name = 'decimal'

    def __init__(self):
        super().__init__(_type=Decimal, errors=DecimalException)


class DecimalRange(RangeParamType):
    name = 'decimal range'

    def __init__(self, minimum: Decimal = None, maximum: Decimal = None, clamp: bool = False):
        super().__init__(DECIMAL, minimum, maximum, clamp)


class DecimalListParamType(ListParamType):
    name = 'decimal list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(DECIMAL, separator=separator, name='decimal values', ignore_empty=ignore_empty)


class FractionParamType(BaseParamType):
    name = 'fraction'

    def __init__(self):
        super().__init__(_type=Fraction, errors=(ValueError, ZeroDivisionError))


class FractionRange(RangeParamType):
    name = 'fraction range'

    def __init__(self, minimum: Fraction = None, maximum: Fraction = None, clamp: bool = False):
        super().__init__(FRACTION, minimum, maximum, clamp)


class FractionListParamType(ListParamType):
    name = 'fraction list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(FRACTION, separator=separator, name='fractions', ignore_empty=ignore_empty)


class ComplexParamType(BaseParamType):
    name = 'complex'

    def __init__(self):
        super().__init__(_type=complex, errors=ValueError)


class ComplexListParamType(ListParamType):
    name = 'complex list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(COMPLEX, separator=separator, name='complex values', ignore_empty=ignore_empty)


class IntListParamType(ListParamType):
    name = 'int list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(click.INT, separator=separator, name='integers', ignore_empty=ignore_empty)


class FloatListParamType(ListParamType):
    name = 'float list'

    def __init__(self, separator: str = ',', ignore_empty: bool = False):
        super().__init__(click.FLOAT, separator=separator, name='floating point values', ignore_empty=ignore_empty)


DECIMAL = DecimalParamType()
FRACTION = FractionParamType()
COMPLEX = ComplexParamType()
