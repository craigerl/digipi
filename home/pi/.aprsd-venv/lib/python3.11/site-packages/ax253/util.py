"""Python ax253 Utility Functions."""
__author__ = "Masen Furer KF7HVM <kf7hvm@0x26.net>"
__copyright__ = "Copyright 2022 Masen Furer and Contributors"
__license__ = "Apache License, Version 2.0"


def valid_length(at_least, at_most=None, sequence_validator=None):
    def _validator(instance, attribute, value):
        if sequence_validator:
            sequence_validator(instance, attribute, value)
        len_value = len(value)
        if len_value < at_least:
            raise ValueError(
                "{} must be at least {} (actual={})".format(
                    attribute.name, at_least, len_value
                )
            )
        if at_most is not None and len_value > at_most:
            raise ValueError(
                "{} must be at most {} (actual={})".format(
                    attribute.name, at_most, len_value
                )
            )

    return _validator


def instance_of_or(types, other_validator):
    """check if the value is a bool or satisfies `other_validator`."""

    def _validator(instance, attribute, value):
        if not isinstance(value, types):
            other_validator(instance, attribute, value)

    return _validator


def optional_bool_or_bytes(v):
    """pass through None and bool; otherwise cast as `bytes`."""
    if v is not None and not isinstance(v, bool):
        return bytes(v)
    return v
