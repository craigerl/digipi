#!/bin/sh
#
# wconversd
#
# chkconfig: 3 91 06
# description: local convers 
# processname: wconversd
# pidfile: /var/run/wconversd.pid
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
	echo -n "Starting wconversd system: "
	daemon /usr/local/sbin/wconversd
	if [ -e /var/run/wconversd.pid ] 
	then
		# conversd pid file resits in /usr/local/etc/conversd
		rm -f /var/run/wconversd.pid
	fi
		ln -s /usr/local/etc/conversd/wconversd.pid /var/run/wconversd.pid
	touch /var/lock/subsys/wconversd
	echo
	;;
  stop)
	if [ -L /var/run/wconversd.pid ]
	then
		echo -n "Stopping wconversd service: "
		killproc wconversd
		rm -f /var/run/wconversd.pid
		echo
	fi
	rm -f /var/lock/subsys/wconversd 
	;;
  status)
	status wconversd
	;;
  *)
	echo "Usage: $0 {start|stop|status}"
	exit 1
	;;
esac

exit 0
