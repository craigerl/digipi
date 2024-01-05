"""Python KISS Module Class Definitions."""

from typing import Any, Union

from ax253 import SyncFrameDecode

from . import constants, kiss, util

__author__ = "Greg Albrecht W2GMD <oss@undef.net>"
__copyright__ = "Copyright 2017 Greg Albrecht and Contributors"
__license__ = "Apache License, Version 2.0"


class KISS(SyncFrameDecode):
    """KISS Object representing a TNC."""

    decode_class = kiss.KISSDecode
    _logger = util.getLogger(__name__)

    def __init__(self, strip_df_start: bool = False) -> None:
        super().__init__()
        self.decoder = self.decode_class(strip_df_start=strip_df_start)

    def write_setting(self, name: str, value: Union[bytes, int]) -> None:
        """
        Writes KISS Command Codes to attached device.

        http://en.wikipedia.org/wiki/KISS_(TNC)#Command_Codes

        :param name: KISS Command Code Name as a string.
        :param value: KISS Command Code Value to write.
        """
        self._logger.debug("Configuring %s=%s", name, repr(value))

        return self.protocol.write_setting(getattr(kiss.Command, name), value)

    def _write_defaults(self, **kwargs: Any) -> None:
        """
        Previous verious defaulted to Xastir-friendly configs. Unfortunately
        those don't work with Bluetooth TNCs, so we're reverting to None.

        Use `config_xastir()` for Xastir defaults.
        """
        for k, v in kwargs.items():
            self.write_setting(k, v)


class TCPKISS(KISS):
    """KISS TCP Class."""

    def __init__(self, host: str, port: int, strip_df_start: bool = False):
        super().__init__(strip_df_start)
        self.address = (host, int(port))

    def stop(self) -> None:
        self.protocol.transport.close()

    def start(self, **kwargs: Any) -> None:
        """
        Initializes the KISS device and commits configuration.
        """
        _, self.protocol = self.loop.run_until_complete(
            kiss.create_tcp_connection(
                *self.address,
                protocol_kwargs={"decoder": self.decoder},
            ),
        )
        self.loop.run_until_complete(self.protocol.connection_future)
        self._write_defaults(**kwargs)


class SerialKISS(KISS):
    """KISS Serial Class."""

    def __init__(self, port: str, speed: str, strip_df_start: bool = False) -> None:
        self.port = port
        self.speed = speed
        super().__init__(strip_df_start)

    def config_xastir(self) -> None:
        """
        Helper method to set default configuration to those that ship with
        Xastir.
        """
        self._write_defaults(**constants.DEFAULT_KISS_CONFIG_VALUES)

    def kiss_on(self) -> None:
        """Turns KISS ON."""
        self.protocol.kiss_on()

    def kiss_off(self) -> None:
        """Turns KISS OFF."""
        self.protocol.kiss_off()

    def stop(self) -> None:
        self.protocol.transport.close()

    def start_no_config(self) -> None:
        """
        Initializes the KISS device without writing configuration.
        """
        _, self.protocol = self.loop.run_until_complete(
            kiss.create_serial_connection(
                port=self.port,
                baudrate=int(self.speed),
                protocol_kwargs={"decoder": self.decoder},
            ),
        )
        self.loop.run_until_complete(self.protocol.connection_future)

    def start(self, **kwargs: Any) -> None:
        """
        Initializes the KISS device and commits configuration.

        See http://en.wikipedia.org/wiki/KISS_(TNC)#Command_codes
        for configuration names.

        :param **kwargs: name/value pairs to use as initial config values.
        """
        self._logger.debug("kwargs=%s", kwargs)
        self.start_no_config()
        self._write_defaults(**kwargs)
