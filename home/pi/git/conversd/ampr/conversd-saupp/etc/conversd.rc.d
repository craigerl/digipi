#!/bin/sh
#
# conversd
#
# chkconfig: 3 91 06
# description: local convers 
# processname: conversd
# pidfile: /var/run/conversd.pid
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
	echo -n "Starting conversd system: "
	daemon /usr/local/sbin/conversd
	if [ -e /var/run/conversd.pid ] 
	then
		# conversd pid file resits in /usr/local/etc/conversd
		rm -f /var/run/conversd.pid
	fi
		ln -s /usr/local/etc/conversd/conversd.pid /var/run/conversd.pid
	touch /var/lock/subsys/conversd
	echo
	;;
  stop)
	if [ -L /var/run/conversd.pid ]
	then
		echo -n "Stopping conversd service: "
		killproc conversd
		rm -f /var/run/conversd.pid
		echo
	fi
	rm -f /var/lock/subsys/conversd 
	;;
  status)
	status conversd
	;;
  *)
	echo "Usage: $0 {start|stop|status}"
	exit 1
	;;
esac

exit 0
