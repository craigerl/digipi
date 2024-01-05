"""Module containing implementations for GCRA."""
import datetime
import math

from . import base
from .. import limit_data
from .. import quota
from .. import result


class GenericCellRatelimiter(base.BaseLimiter):
    """A Generic Cell Ratelimit Algorithm implementation in Pure Python."""

    def rate_limit(
        self, key: str, quantity: int, rate: quota.Quota
    ) -> result.RateLimitResult:
        """Apply the rate-limit to a quantity of requests."""
        # Emission interval is how much is allowed per period
        emission_interval: datetime.timedelta = rate.period / rate.limit
        limit = rate.limit
        delay_variation_tolerance: datetime.timedelta = (
            emission_interval * limit
        )
        # The increment uses the emission interval to find out how much time
        # quantity should have been issued over
        increment: datetime.timedelta = emission_interval * quantity
        now = self.store.current_time()
        data = self.store.get(key)
        # tat is short for theoretical arrival time, we store this as
        # "time" on our limit data
        tat = getattr(data, "time", None) or now
        new_tat = max(now, tat) + increment
        # The theoretical arrival time defines the end-period (in the future)
        # of our bucket. We want to find, however, which is later - our tat or
        # now. Once we have that, we can find the earliest point in our bucket
        # which is the time we would start allowing new requests.
        allow_at: datetime.datetime = new_tat - delay_variation_tolerance
        # Okay, we have our tat, the time we would have started allowing new
        # requests, and present time so let's figure out the difference from
        # our earliest point in time and now.
        distance_from_start_of_bucket = now - allow_at
        # Now that we know how far we are from the start, let's find out how
        # much we have remaining in our quota.
        remaining = math.floor(
            (distance_from_start_of_bucket / emission_interval) + 0.5
        )
        # We also need to calculate the next reset_after for the user
        reset_after = tat - now
        if reset_after.total_seconds() == 0:
            reset_after = datetime.timedelta(seconds=-1)

        if remaining < 1:
            # It's possible that distance_from_start_of_bucket is negative so
            # it is then also possible that there are fewer than 0 remaining
            # requests available. In that case, we're going to be ratelimited.
            remaining = 0
            limited = True
            retry_after = emission_interval - distance_from_start_of_bucket
            new_time = tat
        else:
            limited = False
            retry_after = datetime.timedelta(seconds=-1)
            new_time = new_tat

        used = rate.limit - remaining
        if data is not None:
            limitdata = data.copy_with(
                used=used, remaining=remaining, time=new_time
            )
        else:
            limitdata = limit_data.LimitData(
                used=used, remaining=remaining, created_at=now, time=new_time
            )

        self.store.compare_and_swap(key=key, old=data, new=limitdata)

        return result.RateLimitResult(
            limit=rate.count,
            limited=limited,
            remaining=remaining,
            reset_after=reset_after,
            retry_after=retry_after,
        )

    def reset(self, key: str, rate: quota.Quota) -> result.RateLimitResult:
        """Reset the rate-limit for a given key."""
        now = self.store.current_time()
        reset_tat = now - (rate.period * 2)
        data = limit_data.LimitData(
            used=0, remaining=rate.limit, created_at=now, time=reset_tat
        )
        self.store.set(key=key, data=data)
        return result.RateLimitResult(
            limit=rate.count,
            limited=False,
            remaining=rate.limit,
            reset_after=datetime.timedelta(seconds=-1),
            retry_after=datetime.timedelta(seconds=-1),
        )
