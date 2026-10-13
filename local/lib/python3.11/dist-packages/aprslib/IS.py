# aprslib - Python library for working with APRS
# Copyright (C) 2013-2014 Rossen Georgiev
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

"""
IS class is used for connection to APRS-IS network
"""
import socket
import select
import time
import logging

from . import __version__, string_type, is_py3
from .parsing import parse
from .exceptions import (
    GenericError,
    ConnectionDrop,
    ConnectionError,
    LoginError,
    ParseError,
    UnknownFormat,
    )

__all__ = ['IS']

logging.addLevelName(11, "ParseError")
logging.addLevelName(9, "UnknownFormat")


class IS(object):
    """
    The IS class is used to connect to aprs-is network and listen to the stream
    of packets. You can either run them through aprs.parse() or get them in raw
    form.

    Note: sending of packets is not supported yet

    """
    def __init__(self, callsign, passwd="-1", host="rotate.aprs.net", port=10152):
        """
        callsign        - used when login in
        passwd          - for verification, or "-1" if only listening
        Host & port     - aprs-is server
        """

        self.logger = logging.getLogger(__name__)
        self._parse = parse

        self.set_server(host, port)
        self.set_login(callsign, passwd)

        self.sock = None
        self.filter = ""  # default filter, everything

        self._connected = False
        self.buf = b''

    def _sendall(self, text):
        if is_py3:
            text = text.encode('utf-8')
        self.sock.sendall(text)

    def set_filter(self, filter_text):
        """
        Set a specified aprs-is filter for this connection
        """
        self.filter = filter_text

        self.logger.info("Setting filter to: %s", self.filter)

        if self._connected:
            self._sendall("#filter %s\r\n" % self.filter)

    def set_login(self, callsign, passwd):
        """
        Set callsign and password
        """
        self.callsign = callsign
        self.passwd = passwd

    def set_server(self, host, port):
        """
        Set server ip/host and port to use
        """
        self.server = (host, port)

    def connect(self, blocking=False, retry=30):
        """
        Initiate connection to APRS server and attempt to login

        blocking = False     - Should we block until connected and logged-in
        retry = 30           - Retry interval in seconds
        """

        if self._connected:
            return

        while True:
            try:
                self._connect()
                self._send_login()
                break
            except (LoginError, ConnectionError):
                if not blocking:
                    raise

            self.logger.info("Retrying connection is %d seconds." % retry)
            time.sleep(retry)

    def close(self):
        """
        Closes the socket
        Called internally when Exceptions are raised
        """

        self._connected = False
        self.buf = b''

        if self.sock is not None:
            self.sock.close()

    def sendall(self, line):
        """
        Send a line, or multiple lines sperapted by '\\r\\n'
        """
        if not isinstance(line, string_type):
            raise TypeError("Expected line to be str, got %s", type(line))
        if not self._connected:
            raise ConnectionError("not connected")

        if line == "":
            return

        line = line.rstrip("\r\n") + "\r\n"

        try:
            self.sock.setblocking(1)
            self.sock.settimeout(5)
            self._sendall(line)
        except socket.error as exp:
            self.close()
            raise ConnectionError(str(exp))

    def consumer(self, callback, blocking=True, immortal=False, raw=False):
        """
        When a position sentence is received, it will be passed to the callback function

        blocking: if true (default), runs forever, otherwise will return after one sentence
                  You can still exit the loop, by raising StopIteration in the callback function

        immortal: When true, consumer will try to reconnect and stop propagation of Parse exceptions
                  if false (default), consumer will return

        raw: when true, raw packet is passed to callback, otherwise the result from aprs.parse()
        """

        if not self._connected:
            raise ConnectionError("not connected to a server")

        line = ''

        while True:
            try:
                for line in self._socket_readlines(blocking):
                    if line[0] != "#":
                        if raw:
                            callback(line)
                        else:
                            callback(self._parse(line))
                    else:
                        self.logger.debug("Server: %s", line)
            except ParseError as exp:
                self.logger.log(11, "%s\n    Packet: %s", exp.message, exp.packet)
            except UnknownFormat as exp:
                self.logger.log(9, "%s\n    Packet: %s", exp.message, exp.packet)
            except LoginError as exp:
                self.logger.error("%s: %s", exp.__class__.__name__, exp.message)
            except (KeyboardInterrupt, SystemExit):
                raise
            except (ConnectionDrop, ConnectionError):
                self.close()

                if not immortal:
                    raise
                else:
                    self.connect(blocking=blocking)
                    continue
            except GenericError:
                pass
            except StopIteration:
                break
            except:
                self.logger.error("APRS Packet: %s", line)
                raise

            if not blocking:
                break

    def _open_socket(self):
        """
        Creates a socket
        """
        self.sock = socket.create_connection(self.server, 15)

    def _connect(self):
        """
        Attemps connection to the server
        """

        self.logger.info("Attempting connection to %s:%s", self.server[0], self.server[1])

        try:
            self._open_socket()

            raddr, rport = self.sock.getpeername()

            self.logger.info("Connected to %s:%s", raddr, rport)

            # 5 second timeout to receive server banner
            self.sock.setblocking(1)
            self.sock.settimeout(5)

            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            banner = self.sock.recv(512)
            if is_py3:
                banner = banner.decode('latin-1')

            if banner[0] == "#":
                self.logger.debug("Banner: %s", banner.rstrip())
            else:
                raise ConnectionError("invalid banner from server")

        except ConnectionError as e:
            self.logger.error(str(e))
            self.close()
            raise
        except (socket.error, socket.timeout) as e:
            self.close()

            self.logger.error("Socket error: %s" % str(e))
            if str(e) == "timed out":
                raise ConnectionError("no banner from server")
            else:
                raise ConnectionError(e)

        self._connected = True

    def _send_login(self):
        """
        Sends login string to server
        """
        login_str = "user {0} pass {1} vers aprslib {3}{2}\r\n"
        login_str = login_str.format(
            self.callsign,
            self.passwd,
            (" filter " + self.filter) if self.filter != "" else "",
            __version__
            )

        self.logger.info("Sending login information")

        try:
            self._sendall(login_str)
            self.sock.settimeout(5)
            test = self.sock.recv(len(login_str) + 100)
            if is_py3:
                test = test.decode('latin-1')
            test = test.rstrip()

            self.logger.debug("Server: %s", test)

            (x, x, callsign, status, x) = test.split(' ', 4)

            if callsign == "":
                raise LoginError("Server responded with empty callsign???")
            if callsign != self.callsign:
                raise LoginError("Server: %s" % test)
            if status != "verified," and self.passwd != "-1":
                raise LoginError("Password is incorrect")

            if self.passwd == "-1":
                self.logger.info("Login successful (receive only)")
            else:
                self.logger.info("Login successful")

        except LoginError as e:
            self.logger.error(str(e))
            self.close()
            raise
        except:
            self.close()
            self.logger.error("Failed to login")
            raise LoginError("Failed to login")

    def _socket_readlines(self, blocking=False):
        """
        Generator for complete lines, received from the server
        """
        try:
            self.sock.setblocking(0)
        except socket.error as e:
            self.logger.error("socket error when setblocking(0): %s" % str(e))
            raise ConnectionDrop("connection dropped")

        while True:
            short_buf = b''
            newline = b'\r\n'

            select.select([self.sock], [], [], None if blocking else 0)

            try:
                short_buf = self.sock.recv(4096)

                # sock.recv returns empty if the connection drops
                if not short_buf:
                    self.logger.error("socket.recv(): returned empty")
                    raise ConnectionDrop("connection dropped")
            except socket.error as e:
                self.logger.error("socket error on recv(): %s" % str(e))
                if "Resource temporarily unavailable" in str(e):
                    if not blocking:
                        if len(self.buf) == 0:
                            break

            self.buf += short_buf

            while newline in self.buf:
                line, self.buf = self.buf.split(newline, 1)

                yield line
