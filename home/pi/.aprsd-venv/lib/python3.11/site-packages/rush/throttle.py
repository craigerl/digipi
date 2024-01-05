"""The main throttle interface."""
import attr

from rush import limiters
from rush import quota
from rush import result


@attr.s
class Throttle:
    """The class that acts as the primary interface for throttles.

    This class requires the intantiated rate quota and limiter and handles
    passing the right arguments to the limiter.

    .. attribute:: limiter

        The instance of the rate limiting algorithm that should be used by
        the throttle.

    .. attribute:: rate

        The instantiated :class:`~rush.quota.Quota` that tells the throttle
        and limiter what the limits and periods are for rate limiting.
    """

    rate: quota.Quota = attr.ib()
    limiter: limiters.BaseLimiter = attr.ib()

    def check(self, key: str, quantity: int) -> result.RateLimitResult:
        """Check if the user should be rate limited.

        :param str key:
            The key to use for rate limiting.
        :param int quantity:
            How many resources is being requested against the rate limit.
        :returns:
            The result of calculating whether the user should be rate-limited.
        :rtype:
            :class:`~rush.result.RateLimitResult`
        """
        return self.limiter.rate_limit(key, quantity, self.rate)

    def clear(self, key: str) -> result.RateLimitResult:
        """Clear any existing limits for the given key.

        :param str key:
            The key to use for rate limiting that should be cleared.
        :returns:
            The result of resetting the rate-limit.
        :rtype:
            :class:`~rush.result.RateLimitResult`
        """
        return self.limiter.reset(key, self.rate)

    def peek(self, key: str) -> result.RateLimitResult:
        """Peek at the user's current rate-limit usage.

        .. note::

            This is equivalent to calling :meth:`check` with a quantity of 0.

        :param str key:
            The key to use for rate limiting.
        :returns:
            The current rate-limit usage.
        :rtype:
            :class:`~rush.result.RateLimitResult`
        """
        return self.limiter.rate_limit(key, 0, self.rate)
