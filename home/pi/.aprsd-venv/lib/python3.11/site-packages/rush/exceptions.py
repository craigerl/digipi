"""Exceptions for the rush library."""


class RushError(Exception):
    """Base class for every other Rush-generated exception."""


class AtomicOperationError(RushError):
    """An error occurred trying to guarantee atomicity."""


class MismatchedDataError(AtomicOperationError):
    """An error occurred while swapping data."""

    def __init__(self, message, *, expected_limit_data, actual_limit_data):
        """Handle extra arguments for easier access by users."""
        super().__init__(message)
        self.expected_limit_data = expected_limit_data
        self.actual_limit_data = actual_limit_data


class DataChangedInStoreError(AtomicOperationError):
    """The limit data changed while trying to preserve atomicity."""

    def __init__(self, message, *, original_exception):
        """Handle extra arguments for easier access by users."""
        super().__init__(message)
        self.original_exception = original_exception


class RedisStoreError(RushError):
    """Base class for all RedisStore-related exceptions."""


class InvalidRedisURL(RedisStoreError):
    """The URL provided to connect to Redis is invalid."""

    def __init__(self, message, *, url, error):
        """Handle extra arguments for easier access by users."""
        super().__init__(message)
        self.url = url
        self.error = error
