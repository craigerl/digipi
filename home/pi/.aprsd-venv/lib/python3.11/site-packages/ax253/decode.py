"""Generic deframing decoder."""
import abc
import asyncio
from types import TracebackType
from typing import (
    Any,
    AsyncIterable,
    Callable,
    cast,
    Generic,
    Iterable,
    Optional,
    Sequence,
    SupportsBytes,
    Type,
    TypeVar,
)

from attrs import define, field

__author__ = "Masen Furer KF7HVM <kf7hvm@0x26.net>"
__copyright__ = "Copyright 2022 Masen Furer and Contributors"
__license__ = "Apache License, Version 2.0"


# indicates that no more frames will appear on this protocol
EOF = object()

# Used throughout this module to represent the type of the decoded frame
_T = TypeVar("_T")


@define
class GenericDecoder(Generic[_T]):
    """Generic stateful decoder."""

    @staticmethod
    def decode_frames(frame: bytes) -> Iterable[_T]:
        """
        Decode a single deframed byte chunk.

        :param frame: should represent a single higher level frame to
            decode in some way.
        """
        yield cast(_T, frame)

    def update(self, new_data: bytes) -> Iterable[_T]:
        """
        Decode the next sequence of bytes from the stream.

        :param new_data: the next bytes from the stream
        :return: an iterable of decoded frames
        """
        yield cast(_T, new_data)

    def flush(self) -> Iterable[_T]:
        """Call when the stream is closing to decode any final buffered bytes."""
        if None:
            yield
        return


@define
class FrameDecodeProtocol(asyncio.Protocol, Generic[_T]):
    """Protocol which uses a GenericDecoder to split the stream into frames."""

    transport: Optional[asyncio.Transport] = field(default=None)
    decoder: GenericDecoder[_T] = field(factory=GenericDecoder)
    frames: asyncio.Queue = field(factory=asyncio.Queue, init=False)
    connection_future: asyncio.Future = field(
        factory=asyncio.Future,
        init=False,
    )

    def _queue_frame(self, frame: _T) -> None:
        self.frame_decoded(frame)
        self.frames.put_nowait(frame)

    def connection_made(self, transport: asyncio.Transport) -> None:
        """
        asyncio callback when connection is established.

        Because this protocol exposes higher-level read/write operations that
        require the transport, an awaitable `connection_future` is completed for consumers
        who depend on an active connection.
        """
        self.transport = transport
        self.connection_future.set_result(transport)

    def frame_decoded(self, frame: _T) -> None:
        """Subclasses may override this function to handle new frame."""
        pass

    def data_received(self, data: bytes) -> None:
        """Pass data off to decoder instance and put frames on the queue."""
        for frame in self.decoder.update(data):
            self._queue_frame(frame)

    def connection_lost(self, exc: Exception) -> None:
        """asyncio callback when connection is lost."""
        for frame in self.decoder.flush():
            self._queue_frame(frame)
        self.frames.put_nowait(EOF)

    async def read(
        self,
        n_frames: Optional[int] = None,
        callback: Optional[Callable[[_T], None]] = None,
    ) -> AsyncIterable[_T]:
        """
        Iterate through decoded frames.

        If n_frames is specified, exit after yielding that number of frames.
        """
        if n_frames is None:
            n_frames = -1
        transport = await self.connection_future
        while (not transport.is_closing() or not self.frames.empty()) and n_frames:
            frame = await self.frames.get()
            if frame is EOF:
                break
            if callback is not None:
                callback(frame)
            yield frame
            n_frames -= 1

    def read_frames(
        self,
        n_frames: Optional[int] = -1,
        callback: Optional[Callable[[_T], None]] = None,
        loop: Optional[asyncio.BaseEventLoop] = None,
    ) -> Sequence[_T]:
        """Blocking read of the given number of frames."""
        if loop is None:
            loop = asyncio.get_event_loop()

        if n_frames is not None and n_frames < 0:
            n_frames = self.frames.qsize()

        async def _():
            return [f async for f in self.read(n_frames=n_frames, callback=callback)]

        return loop.run_until_complete(_())

    def write(self, frame: SupportsBytes) -> None:
        """
        Write serialized frame to the underlying transport.

        :param frame: Frame to write.
        """
        self.transport.write(bytes(frame))


@define
class SyncFrameDecode(abc.ABC, Generic[_T]):
    """
    Synchronous wrapper over FrameDecodeProtocol.

    It is recommended to use this class as a contextmanager for automatic
    start/stop functionality.
    """

    _loop = None
    """The asyncio event loop that this class and subclasses will use."""

    _protocol: Optional[FrameDecodeProtocol[_T]] = field(default=None)
    """The connected protocol (access via .protocol property)."""

    @property
    def loop(self) -> asyncio.BaseEventLoop:
        """Get a reference to a shared event loop for this class."""
        if SyncFrameDecode._loop is None:
            try:
                SyncFrameDecode._loop = asyncio.get_running_loop()
            except RuntimeError:
                SyncFrameDecode._loop = asyncio.new_event_loop()
                asyncio.set_event_loop(SyncFrameDecode._loop)
        return SyncFrameDecode._loop

    @property
    def protocol(self) -> FrameDecodeProtocol[_T]:
        if self._protocol is None:
            raise IOError(
                "Underlying connection is not active. Hint: did you call start()?",
            )
        return self._protocol

    @protocol.setter
    def protocol(self, p) -> None:
        if not isinstance(p, FrameDecodeProtocol):
            raise ValueError(
                "Protocol must be a subclass of {!r}, got {!r}".format(
                    FrameDecodeProtocol, p
                ),
            )
        self._protocol = p

    def __enter__(self) -> "SyncFrameDecode":
        self.start()
        return self

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc: Optional[BaseException],
        traceback: Optional[TracebackType],
    ) -> Optional[bool]:
        return self.stop()

    def __del__(self) -> None:
        return self.stop()

    @abc.abstractmethod
    def start(self, **kwargs: Any) -> None:
        """
        Method is responsible for establishing the connection and assigning self.protocol.

        If self.protocol isn't assigned, the SyncFrameDecode is considered closed.
        """

    @abc.abstractmethod
    def stop(self) -> None:
        """
        Method is responsible for closing the transport.
        """

    def read(
        self,
        callback: Optional[Callable[[_T], None]] = None,
        min_frames: Optional[int] = -1,
    ) -> Sequence[_T]:
        """
        Read frames from underlying protocol.

        :param callback: optional function is called after each frame is decoded.
            The callback will be fired for already-decoded frames immediately,
            and then for any frames that are decoded while blocked.
        :param min_frames: block until this minimum number of frames are available.
            if -1 (default), return all buffered frames without blocking
            if None, read until EOF is seen (device closed)
        :return: List of frames
        """
        if self.protocol is None:
            raise EOFError(
                "Underlying connection is not active. Hint: did you call start()?",
            )
        return list(
            self.protocol.read_frames(
                n_frames=min_frames,
                callback=callback,
                loop=self.loop,
            )
        )

    def write(self, frame: SupportsBytes) -> None:
        """
        Writes frame to KISS interface.

        :param frame: Frame to write.
        """
        self.protocol.write(frame)
