"""Module containing implementations for GCRA."""
import datetime
import typing

import attr

from . import base
from .. import quota
from .. import result
from ..stores import redis

# Copied from
# https://github.com/rwz/redis-gcra/blob/d6723797d3353ff0e607eb96235b3ec5b1135fd7/vendor/perform_gcra_ratelimit.lua
APPLY_RATELIMIT_LUA = """
-- this script has side-effects, so it requires replicate commands mode
redis.replicate_commands()

local rate_limit_key = KEYS[1]
local burst = ARGV[1]
local rate = ARGV[2]
local period = ARGV[3]
local cost = ARGV[4]

local emission_interval = period / rate
local increment = emission_interval * cost
local burst_offset = emission_interval * burst
local now = redis.call("TIME")

-- redis returns time as an array containing two integers: seconds of the epoch
-- time (10 digits) and microseconds (6 digits). for convenience we need to
-- convert them to a floating point number. the resulting number is 16 digits,
-- bordering on the limits of a 64-bit double-precision floating point number.
-- adjust the epoch to be relative to Jan 1, 2017 00:00:00 GMT to avoid
-- floating point problems. this approach is good until "now" is 2,483,228,799
-- (Wed, 09 Sep 2048 01:46:39 GMT), when the adjusted value is 16 digits.
local jan_1_2017 = 1483228800
now = (now[1] - jan_1_2017) + (now[2] / 1000000)

local tat = redis.call("GET", rate_limit_key)

if not tat then
  tat = now
else
  tat = tonumber(tat)
end

local new_tat = math.max(tat, now) + increment

local allow_at = new_tat - burst_offset
local diff = now - allow_at

local limited
local retry_after
local reset_after

-- poor person's round
local remaining = math.floor(diff / emission_interval + 0.5)

if remaining < 0 then
  limited = 1
  remaining = 0
  reset_after = tat - now
  retry_after = diff * -1
else
  limited = 0
  reset_after = new_tat - now
  redis.call("SET", rate_limit_key, new_tat, "EX", math.ceil(reset_after))
  retry_after = -1
end

return {limited, remaining, tostring(retry_after), tostring(reset_after)}
"""

# Copied from
# https://github.com/rwz/redis-gcra/blob/d6723797d3353ff0e607eb96235b3ec5b1135fd7/vendor/inspect_gcra_ratelimit.lua
CHECK_RATELIMIT_LUA = """
local rate_limit_key = KEYS[1]
local burst = ARGV[1]
local rate = ARGV[2]
local period = ARGV[3]

local emission_interval = period / rate
local burst_offset = emission_interval * burst
local now = redis.call("TIME")

-- redis returns time as an array containing two integers: seconds of the epoch
-- time (10 digits) and microseconds (6 digits). for convenience we need to
-- convert them to a floating point number. the resulting number is 16 digits,
-- bordering on the limits of a 64-bit double-precision floating point number.
-- adjust the epoch to be relative to Jan 1, 2017 00:00:00 GMT to avoid
-- floating point problems. this approach is good until "now" is 2,483,228,799
-- (Wed, 09 Sep 2048 01:46:39 GMT), when the adjusted value is 16 digits.
local jan_1_2017 = 1483228800
now = (now[1] - jan_1_2017) + (now[2] / 1000000)

local tat = redis.call("GET", rate_limit_key)

if not tat then
  tat = now
else
  tat = tonumber(tat)
end

local allow_at = math.max(tat, now) - burst_offset
local diff = now - allow_at

-- poor person's round
local remaining = math.floor(diff / emission_interval + 0.5)

local reset_after = tat - now
if reset_after == 0 then
  reset_after = -1
end

local limited
local retry_after

if remaining < 1 then
  remaining = 0
  limited = 1
  retry_after = emission_interval - diff
else
  limited = 0
  retry_after = -1
end

return {limited, remaining, tostring(retry_after), tostring(reset_after)}
"""


@attr.s
class GenericCellRatelimiter(base.BaseLimiter):
    """A Generic Cell Ratelimit Algorithm implementation in Redis LUA."""

    store: redis.RedisStore = attr.ib(
        validator=attr.validators.instance_of(redis.RedisStore)
    )

    def __attrs_post_init__(self):
        """Configure our redis client based off our store."""
        self.client = self.store.client
        self.check_ratelimit = self.client.register_script(
            CHECK_RATELIMIT_LUA
        )
        self.apply_ratelimit = self.client.register_script(
            APPLY_RATELIMIT_LUA
        )

    def _call_lua(
        self,
        *,
        keys: typing.List[str],
        cost: int,
        burst: int,
        rate: float,
        period: float,
    ) -> typing.Tuple[int, int, str, str]:
        if cost == 0:
            return self.check_ratelimit(keys=keys, args=[burst, rate, period])
        else:
            return self.apply_ratelimit(
                keys=keys, args=[burst, rate, period, cost]
            )

    def rate_limit(
        self, key: str, quantity: int, rate: quota.Quota
    ) -> result.RateLimitResult:
        """Apply the rate-limit to a quantity of requests."""
        period = rate.period.total_seconds()
        limited, remaining, retry_after_s, reset_after_s = self._call_lua(
            keys=[key],
            cost=quantity,
            burst=rate.limit,
            rate=rate.count / period,
            period=period,
        )
        retry_after = datetime.timedelta(seconds=float(retry_after_s))
        reset_after = datetime.timedelta(seconds=float(reset_after_s))

        return result.RateLimitResult(
            limit=rate.limit,
            limited=limited == 1,
            remaining=remaining,
            retry_after=retry_after,
            reset_after=reset_after,
        )

    def reset(self, key: str, rate: quota.Quota) -> result.RateLimitResult:
        """Reset the rate-limit for a given key."""
        self.client.delete(key)
        return result.RateLimitResult(
            limit=rate.limit,
            limited=False,
            remaining=rate.count,
            reset_after=datetime.timedelta(seconds=-1),
            retry_after=datetime.timedelta(seconds=-1),
        )
