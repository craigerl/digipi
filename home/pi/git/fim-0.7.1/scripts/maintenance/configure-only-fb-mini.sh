#!/bin/sh
# Configure fim with minimal features, with no X
./configure --disable-sdl $@   || exit
make || exit
make tests || exit
