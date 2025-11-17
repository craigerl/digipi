#!/bin/sh
# configure fim with scripting disabled (still incomplete, though)
./configure --disable-readline --disable-system --disable-autocommands --disable-windows --disable-scripting --disable-output-console  $@ || exit 
make || exit
make tests || exit
