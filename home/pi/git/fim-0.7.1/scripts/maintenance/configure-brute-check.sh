#!/bin/sh

# this script brute-check for ./configure --enable.. and ./configure --disable.. options 

C=./configure
CH="$C --help=short"

FAILED=""
OK=""
#CXXFLAGS="-O0"
CXXFLAGS="-pedantic -Wall"

trap '

p "OK     : $OK";
p "FAILED : $FAILED";
p "ALL   : $ALL"' EXIT

err() { echo "[!] $@" ; exit -1; }
p() { echo "[*] $@" ; }

$CH || err "problems with $C"

ALL=$(
for W in disable enable  ; do
for S in `./configure --help=short | grep $W | grep -v "$W-[A-Z]" | sed 's/^\s*//g;s/\s\+.*//g'` ; do
	echo -n " $S" 
done
done
)
for S in $ALL
do
	{ $C $S && make clean && make CXXFLAGS=$CXXFLAGS && { OK="$OK $S" ; } } || { FAILED="$FAILED $S" ; }
done

