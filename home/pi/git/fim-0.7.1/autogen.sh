#!/bin/sh
# $LastChangedDate: 2011-05-23 14:51:20 +0200 (Mon, 23 May 2011) $

# This file is still not complete.

# this should create configure.scan

aclocal  || { echo "no aclocal  ?" ; exit 1 ; }

#autoscan

# we produce suitable config.h.in
autoheader

# we produce a configure script
autoconf || { echo "no autoconf ?" ; exit 1 ; }

# we produce a brand new Makefile
automake --add-missing || { echo "no automake ?" ; exit 1 ; }

# The automake required for autogen.sh'in this package is 1.10.
# So users who want to build from the svn repository are required to use this version.
# 
# Users building from the tarball shouldn't bother, of course, 
# because they get the configure script generated from the tarball maintainer.

