#!/bin/sh
for i in 1 2; do nice -n 20 openssl speed >/dev/null 2>&1 & done
