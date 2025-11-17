#!/bin/sh
# Configure fim for a live system (that is, reading no configuration file at all, and writing out no history).
./configure --disable-fimrc --disable-history $@   || exit
make || exit
make tests || exit
