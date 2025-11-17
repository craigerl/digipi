#!/bin/sh
#
# lconversd
#
# chkconfig: 3 91 06
# description: local convers 
# processname: lconversd
# pidfile: /var/run/lconversd.pid
# probe: true

# Source function library.
. /etc/rc.d/init.d/functions

# Get config.
. /etc/sysconfig/network

# Check that networking is up.
if [ ${NETWORKING} = "no" ]
then
	exit 0
fi


# See how we were called.
case "$1" in
  start)
	echo -n "Starting lconversd system: "
	daemon /usr/local/sbin/lconversd
	if [ -e /var/run/lconversd.pid ] 
	then
		# conversd pid file resits in /usr/local/etc/conversd
		rm -f /var/run/lconversd.pid
	fi
		ln -s /usr/local/etc/conversd/lconversd.pid /var/run/lconversd.pid
	touch /var/lock/subsys/lconversd
	echo
	;;
  stop)
	if [ -L /var/run/lconversd.pid ]
	then
		echo -n "Stopping lconversd service: "
		killproc lconversd
		rm -f /var/run/lconversd.pid
		echo
	fi
	rm -f /var/lock/subsys/lconversd 
	;;
  status)
	status lconversd
	;;
  *)
	echo "Usage: $0 {start|stop|status}"
	exit 1
	;;
esac

exit 0
