"""Our rate quota logic."""
import datetime
import typing

import attr

Q = typing.TypeVar("Q", bound="Quota")

_one_second: datetime.timedelta = datetime.timedelta(seconds=1)
_one_minute: datetime.timedelta = datetime.timedelta(minutes=1)
_one_hour: datetime.timedelta = datetime.timedelta(hours=1)
_one_day: datetime.timedelta = datetime.timedelta(days=1)


@attr.s(frozen=True)
class Quota:
    """The definition of a user's quota of resources.

    .. attribute:: period

        The time between equally spaced requests. This must be greater than 0
        seconds.

    .. attribute:: count

        The number of requests to a resource allowed in the period. This must
        be greater than 0.

    .. attribute:: maximum_burst

        The number of requests that will be allowed to exceed the rate in a
        single burst. This must be greater than or equal to 0 and defaults to
        0.

    """

    period: datetime.timedelta = attr.ib()
    count: int = attr.ib()
    maximum_burst: int = attr.ib(default=0)

    @period.validator
    def _period_is_positive(
        self, attribute, period: datetime.timedelta
    ) -> None:
        if period.total_seconds() <= 0:
            raise ValueError("The period must be a positive value.")

    @count.validator
    def _count_is_positive(self, attribute, count: int) -> None:
        if count < 0:
            raise ValueError("The quota's count must be non-negative.")

    @maximum_burst.validator
    def _burst_is_positive(self, attribute, count: int) -> None:
        if count < 0:
            raise ValueError(
                "The quota's maximum_burst must be non-negative."
            )

    @classmethod
    def per_second(
        cls: typing.Type[Q], count: int, *, maximum_burst: int = 0
    ) -> Q:
        """Create a quota based on the number allowed per second.

        :param int count:
            The number of requests allowed per second.
        :returns:
            A new quota.
        :rtype:
            :class:`~rush.throttle.Quota`
        """
        return cls(
            period=_one_second, count=count, maximum_burst=maximum_burst
        )

    @classmethod
    def per_minute(
        cls: typing.Type[Q], count: int, *, maximum_burst: int = 0
    ) -> Q:
        """Create a quota based on the number allowed per minute.

        :param int count:
            The number of requests allowed per minute.
        :returns:
            A new quota.
        :rtype:
            :class:`~rush.throttle.Quota`
        """
        return cls(
            period=_one_minute, count=count, maximum_burst=maximum_burst
        )

    @classmethod
    def per_hour(
        cls: typing.Type[Q], count: int, *, maximum_burst: int = 0
    ) -> Q:
        """Create a quota based on the number allowed per hour.

        :param int count:
            The number of requests allowed per hour.
        :returns:
            A new quota.
        :rtype:
            :class:`~rush.throttle.Quota`
        """
        return cls(period=_one_hour, count=count, maximum_burst=maximum_burst)

    @classmethod
    def per_day(
        cls: typing.Type[Q], count: int, *, maximum_burst: int = 0
    ) -> Q:
        """Create a quota based on the number allowed per day.

        :param int count:
            The number of requests allowed per day.
        :returns:
            A new quota.
        :rtype:
            :class:`~rush.throttle.Quota`
        """
        return cls(period=_one_day, count=count, maximum_burst=maximum_burst)

    @property
    def limit(self) -> int:
        """Return the calculated limit including maximum burst."""
        return self.count + self.maximum_burst
