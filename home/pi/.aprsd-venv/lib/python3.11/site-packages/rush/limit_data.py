"""Logic for our limit dataclass."""
import datetime
import typing

import attr

DATETIME_FORMAT = "%Y-%m-%dT%H:%M:%S.%f%z"


def convert_str_to_datetime(
    value: typing.Union[str, datetime.datetime]
) -> datetime.datetime:
    """Convert datetime strings to datetimes."""
    if isinstance(value, str):
        return datetime.datetime.strptime(value, DATETIME_FORMAT)
    return value


def maybe_convert_str_to_datetime(
    value: typing.Union[str, datetime.datetime]
) -> typing.Optional[datetime.datetime]:
    """Handle empty string for time attribute."""
    if value == "":
        return None
    return convert_str_to_datetime(value)


@attr.s(frozen=True)
class LimitData:
    """Data class that organizes our limit data for storage."""

    used: int = attr.ib(converter=int)
    remaining: int = attr.ib(converter=int)
    created_at: datetime.datetime = attr.ib(converter=convert_str_to_datetime)
    time: typing.Optional[datetime.datetime] = attr.ib(
        converter=attr.converters.optional(maybe_convert_str_to_datetime),
        default=None,
        kw_only=True,
    )

    @created_at.default
    def _now(self):
        return datetime.datetime.now(datetime.timezone.utc)

    def asdict(self) -> typing.Dict[str, str]:
        """Return the data as a dictionary.

        :returns:
            A dictionary mapping the attributes to string representations
            of the values.
        """
        time = ""
        if self.time is not None:
            time = self.time.strftime(DATETIME_FORMAT)
        return {
            "used": str(self.used),
            "remaining": str(self.remaining),
            "created_at": self.created_at.strftime(DATETIME_FORMAT),
            "time": time,
        }

    def copy_with(
        self,
        *,
        used: typing.Optional[int] = None,
        remaining: typing.Optional[int] = None,
        created_at: typing.Optional[datetime.datetime] = None,
        time: typing.Optional[datetime.datetime] = None,
    ) -> "LimitData":
        """Create a copy of this with updated values.

        :param int used:
        :param int remaining:
        :param datetime.datetime created_at:
        :param datetime.datetime time:
        :returns:
            A new copy of this instance with the overridden values.
        :rtype:
            :class:`~rush.stores.base.LimitData`
        """
        used = used if used is not None else self.used
        remaining = remaining if remaining is not None else self.remaining
        created_at = created_at if created_at is not None else self.created_at
        time = time if time is not None else self.time
        return LimitData(
            used=used, remaining=remaining, created_at=created_at, time=time
        )
