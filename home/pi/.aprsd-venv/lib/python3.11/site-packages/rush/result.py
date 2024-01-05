"""Logic for our ratelimit result."""
import datetime
import typing

import attr


@attr.s(frozen=True)
class RateLimitResult:
    """A result of checking a ratelimit.

    The attributes on this object are:

    .. attribute:: limit

        The integer limit that was checked against, e.g., if the user's
        ratelimit is 10,000 this value will be 10,000 regardless of how much
        they have consumed.

    .. attribute:: limited

        Whether or not the user should be ratelimited (a.k.a., throttled).

    .. attribute:: remaining

        The integer representing how much of the user's ratelimit is left.
        This should be the number of requests made during the time period,
        ``N``, subtracted from the limit, ``L``, or ``L - N``.

    .. attribute:: reset_after

        This will be a :class:`~datetime.timedelta` representing how much time
        is left until the ratelimit resets. For example if the ratelimit will
        reset in 800ms then this might look like:

        .. code-block:: python

            datetime.timedelta(0, 0, 800000)
            # == datetime.timedelta(milliseconds=800)

    .. attribute:: retry_after

        This will be a :class:`~datetime.timedelta` representing the length of
        time after which a retry can be made.

    """

    limit: int = attr.ib()
    limited: bool = attr.ib()
    remaining: int = attr.ib()
    reset_after: datetime.timedelta = attr.ib()
    retry_after: datetime.timedelta = attr.ib()

    @staticmethod
    def _now() -> datetime.datetime:
        return datetime.datetime.now(datetime.timezone.utc)

    def resets_at(
        self, from_when: typing.Optional[datetime.datetime] = None
    ) -> datetime.datetime:
        """Calculate the reset time from UTC now.

        :returns:
            The UTC timezone-aware datetime representing when the limit
            resets.
        """
        if from_when is None:
            from_when = self._now()
        return from_when + self.reset_after

    def retry_at(
        self, from_when: typing.Optional[datetime.datetime] = None
    ) -> datetime.datetime:
        """Calculate the retry time from UTC now.

        :returns:
            The UTC timezone-aware datetime representing when the user
            can retry.
        """
        if from_when is None:
            from_when = self._now()
        return from_when + self.retry_after
