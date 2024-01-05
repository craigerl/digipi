"""Module containing the logic for our dictionary store."""
import typing

import attr

from . import base
from .. import exceptions
from .. import limit_data


@attr.s
class DictionaryStore(base.BaseStore):
    """Basic storage for testing that utilizes a dictionary."""

    store: typing.Dict[str, limit_data.LimitData] = attr.ib(factory=dict)

    def compare_and_swap(
        self,
        key: str,
        old: typing.Optional[limit_data.LimitData],
        new: limit_data.LimitData,
    ) -> limit_data.LimitData:
        """Re-retrieve the limit data, compare and swap it.

        .. warning::

            This store makes no guarantees of thread safety or atomicity.
            Thus, it does not use any locking to make this method atomic.

        This returns the old data if the data has changed.
        """
        old_limitdata = self.get(key)
        if old != old_limitdata:
            raise exceptions.MismatchedDataError(
                "old limit data did not match expected limit data",
                expected_limit_data=old,
                actual_limit_data=old_limitdata,
            )
        return self.set(key=key, data=new)

    def get(self, key: str) -> typing.Optional[limit_data.LimitData]:
        """Retrieve the data for a given key."""
        data = self.store.get(key, None)
        return data

    def set(
        self, *, key: str, data: limit_data.LimitData
    ) -> limit_data.LimitData:
        """Store the values for a given key."""
        self.store[key] = data
        return self.store[key]
