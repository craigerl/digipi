#!/bin/sh
set -e
set -x
# Configure fim with minimal features allowing it to run in the framebuffer
./configure --disable-sdl --disable-gtk --disable-fimrc --disable-history --disable-readline \
	--disable-xfig --disable-xcftopnm --disable-xfig --disable-convert \
       --disable-stdin-image-reading --disable-ps --disable-djvu \
       --disable-mouse \
       --disable-raw-bits-rendering --disable-system "$@"
make
make tests
