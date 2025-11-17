#!/bin/bash

# @(#) $Id: online.sh,v 1.2 2022/07/09 20:09:32 dl9sau Exp $

# online and cstat command - as alternative to the C program
# usage:
# online [..convers] [options]
# cstat [..convers] [options]
# map [..convers] [options]

# we could not assume that /etc/services is available
# -> name:port [...]
PORTS="convers:3600 wconvers:3610 lconvers:6810"

# nc timeout: if your computer is old and slow, you it may took more time
#TIMEOUT=1
TIMEOUT=30

# SuSE names it's netcat program netcat, but not nc :-(
# search for the bad name first..

NC=$((which netcat || which nc) 2>/dev/null)

if [ $? -gt 0 ]
then
  echo "error: helper program nc(1) not found"
  exit 1
fi

# our name is online, or cstat
COMMAND=$(basename $0)

if [ $# -gt 0 ]
then

  ONLY=""
  for port in $PORTS
  do
    echo $port | grep "^$1:" >/dev/null
    if [ $? -eq 0 ]
    then
      ONLY=$port
    fi
  done
  if [ x${ONLY} != x ]
  then
    # asked for single conversd by name, and found the port?
    PORTS=$ONLY
    shift
  else
    echo $1 | grep -i convers >/dev/null
    if [ $? -eq 0 ]
    then
    	# asked for name. nc will look in /etc/services
	PORTS=$1
	shift
    fi
  fi

  if [ $# -gt 0 ]
  then
    COMMAND="$COMMAND $*"
  fi
    
fi

for port in $PORTS
do
  # print header
  echo $(echo $port | cut -d: -f1) $(echo $COMMAND | awk '{ print $1}'):
  # get port from name
  port=$(echo $port | cut -d: -f2)
  (echo "/$COMMAND " ; sleep 1; echo /quit) | $NC -w $TIMEOUT 127.0.0.1 $port
done

exit $?
