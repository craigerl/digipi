#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
# DigiPi Craig Lamparter adaptation to start/stop tnc and digipeater

"""Minimal example of asynchronously watching for edges on a single line."""

import gpiod
import select

import subprocess
from signal import pause
from os import getenv
from dotenv import load_dotenv
from pathlib import Path
from os import path
import signal

from datetime import timedelta
from gpiod.line import Bias, Edge

def signal_handler(signal, frame):
   print("Got ", signal, " exiting.")
   os.eventfd_write(done_fd, 1)
   os.close(done_fd)
   os._exit(0)
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


def handle_gpio23():
    try:
        cmd = "sudo systemctl is-active tnc"
        status = "active"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    except:
        status = "inactive"
    if status == "inactive":
        cmd = "sudo systemctl start tnc"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    else:
        cmd = "hostname -I | cut -d' ' -f1"
        IP = subprocess.check_output(cmd, shell=True).decode("utf-8")
        if ( len(IP) < 5 ):
            IP = "0.0.0.0"
        cmd = "sudo systemctl stop tnc"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        subprocess.call(['/home/pi/digibanner.py', '-b', 'Standby', '-s', IP, '-d', getenv('NEWDISPLAYTYPE')])
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")


def handle_gpio24():
    try:
        cmd = "sudo systemctl is-active digipeater"
        status = "active"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    except:
        status = "inactive"
    if status == "inactive":
        cmd = "sudo systemctl start digipeater"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
    else:
        cmd = "hostname -I | cut -d' ' -f1"
        IP = subprocess.check_output(cmd, shell=True).decode("utf-8")
        if ( len(IP) < 5 ):
            IP = "0.0.0.0"
        cmd = "sudo systemctl stop digipeater"
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")
        subprocess.call(['/home/pi/digibanner.py', '-b', 'Standby', '-s', IP, '-d', getenv('NEWDISPLAYTYPE')])
        cmd_output = subprocess.check_output(cmd, shell=True).decode("utf-8")


def edge_type_str(event):
    if event.event_type is event.Type.RISING_EDGE:
        return "Rising"
    if event.event_type is event.Type.FALLING_EDGE:
        return "Falling"
    return "Unknown"


def async_watch_line_23(chip_path, line_offset, done_fd):
    # Assume a button connecting the pin to ground,
    # so pull it up and provide some debounce.
    with gpiod.request_lines(
        chip_path,
        consumer="async-watch-line-value",
        config={
            line_offset: gpiod.LineSettings(
                edge_detection=Edge.BOTH,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=10),
            )
        },
    ) as request:
        poll = select.poll()
        poll.register(request.fd, select.POLLIN)
        # Other fds could be registered with the poll and be handled
        # separately using the return value (fd, event) from poll():
        poll.register(done_fd, select.POLLIN)
        while True:
            for fd, _event in poll.poll():
                if fd == done_fd:
                    # perform any cleanup before exiting...
                    return
                # handle any edge events
                for event in request.read_edge_events():
#                    print(
#                        "offset: {}  type: {:<7}  event #{}".format(
#                            event.line_offset, edge_type_str(event), event.line_seqno
#                        )
#                    )
                    if edge_type_str(event) == "Rising":
                        handle_gpio23()


def async_watch_line_24(chip_path, line_offset, done_fd):
    # Assume a button connecting the pin to ground,
    # so pull it up and provide some debounce.
    with gpiod.request_lines(
        chip_path,
        consumer="async-watch-line-value",
        config={
            line_offset: gpiod.LineSettings(
                edge_detection=Edge.BOTH,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=10),
            )
        },
    ) as request:
        poll = select.poll()
        poll.register(request.fd, select.POLLIN)
        # Other fds could be registered with the poll and be handled
        # separately using the return value (fd, event) from poll():
        poll.register(done_fd, select.POLLIN)
        while True:
            for fd, _event in poll.poll():
                if fd == done_fd:
                    # perform any cleanup before exiting...
                    return
                # handle any edge events
                for event in request.read_edge_events():
#                    print(
#                        "offset: {}  type: {:<7}  event #{}".format(
#                            event.line_offset, edge_type_str(event), event.line_seqno
#                        )
#                    )
                    if edge_type_str(event) == "Rising":
                        handle_gpio24()


if __name__ == "__main__":
    import os
    import threading

    dotenv_path = Path('/home/pi/localize.env')
    load_dotenv(dotenv_path=dotenv_path)

    # run the async executor (select.poll) in a thread to demonstrate a graceful exit.
    done_fd = os.eventfd(0)

    def bg_thread_23():
        try:
            async_watch_line_23("/dev/gpiochip0", 23, done_fd)
        except OSError as ex:
            print(ex, "\nUnable to monitor gpio button 23.")
        print("background thread exiting...")
    t23 = threading.Thread(target=bg_thread_23)
    t23.start()

    def bg_thread_24():
        try:
            async_watch_line_24("/dev/gpiochip0", 24, done_fd)
        except OSError as ex:
            print(ex, "\nUnable to monitor gpio button 24.")
        print("background thread exiting...")
    t24 = threading.Thread(target=bg_thread_24)
    t24.start()

    pause()  

