"""Redis storage logic."""
import datetime
import typing

import attr
import redis
import rfc3986

from . import base
from .. import exceptions
from .. import limit_data

URL_VALIDATOR = (
    rfc3986.validators.Validator()
    .allow_schemes("redis", "rediss", "unix")
    .require_presence_of("scheme")
)


def parse(value: str) -> rfc3986.ParseResult:
    """Convert strings to parsed URIs.

    This papers over the lack of stubs for mypy.
    """
    return rfc3986.urlparse(value)


@attr.s
class RedisStore(base.BaseStore):
    """Logic for storing things in redis."""

    url: rfc3986.ParseResult = attr.ib(converter=parse)
    client_config: typing.Dict[str, typing.Any] = attr.ib(factory=dict)
    client: redis.StrictRedis = attr.ib()

    @url.validator
    def _validate_url(self, attribute, value):
        """Ensure our URL has the bare minimum we need."""
        try:
            URL_VALIDATOR.validate(value.reference)
        except rfc3986.exceptions.ValidationError as err:
            url = value.unsplit()
            raise exceptions.InvalidRedisURL(
                f"Provided URL {url} is invalid for Redis storage.",
                url=url,
                error=err,
            )

    @client.default
    def _make_client(self):
        """Create the Redis client from the URL and config."""
        attr.validate(self)  # Force validation of self.url
        self.client_config.setdefault("decode_responses", True)
        return redis.StrictRedis.from_url(
            url=self.url.unsplit(),
            **self.client_config,
        )

    def compare_and_swap(
        self,
        key: str,
        old: typing.Optional[limit_data.LimitData],
        new: limit_data.LimitData,
    ) -> limit_data.LimitData:
        """Perform an atomic compare and swap operation."""
        with self.client.pipeline() as p:
            try:
                p.watch(key)
                data = p.hgetall(key)
                current_data = limit_data.LimitData(**data) if data else None
                if old != current_data:
                    raise exceptions.MismatchedDataError(
                        "old limit data did not match expected limit data",
                        expected_limit_data=old,
                        actual_limit_data=current_data,
                    )
                p.multi()
                p.hmset(key, new.asdict())
                p.execute()
            except redis.WatchError as we:
                raise exceptions.DataChangedInStoreError(
                    "error swapping the limit data", original_exception=we
                )
        return new

    def set(
        self, *, key: str, data: limit_data.LimitData
    ) -> limit_data.LimitData:
        """Store the values for a given key."""
        datadict = typing.cast(  # Cast until the stubs are fixed
            typing.Mapping[
                typing.Union[bytes, float, int, str],
                typing.Union[bytes, float, int, str],
            ],
            # See also https://stackoverflow.com/a/64484841/1953283 as an
            # explanation of why the redis-py typeshed stubs are wrong
            data.asdict(),
        )
        self.client.hmset(key, datadict)
        return data

    def get(self, key: str) -> typing.Optional[limit_data.LimitData]:
        """Retrieve the data for a given key."""
        data = self.client.hgetall(key)
        return limit_data.LimitData(**data) if data else None

    def current_time(
        self, tzinfo: typing.Optional[datetime.tzinfo] = datetime.timezone.utc
    ) -> datetime.datetime:
        """Return the curent date and time as a datetime.

        This uses Redis's ``TIME`` command to determine the current time.

        :returns:
            Now in UTC
        :retype:
            :class:`~datetime.datetime`
        """
        seconds, microseconds = self.client.time()
        return datetime.datetime.utcfromtimestamp(
            seconds + (microseconds / 1_000_000)
        ).replace(tzinfo=tzinfo)
