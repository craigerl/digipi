#!/bin/sh
mkdir /run/conversd
chown daemon /run/conversd
/usr/local/sbin/conversd
