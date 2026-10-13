#!/bin/sh

tail -n 90 -f /run/direwolf.log | grep -a --line-buffered -E -v  '^(PTT|DCD)' 

