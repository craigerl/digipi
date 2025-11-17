#!/bin/sh
#
# ppconversd
#
# chkconfig: 3 91 06
# description: local convers 
# processname: ppconversd
# pidfile: /var/run/ppconversd.pid
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
	echo -n "Starting ppconversd system: "
	daemon /usr/local/sbin/ppconversd
	if [ -e /var/run/ppconversd.pid ] 
	then
		# conversd pid file resits in /usr/local/etc/conversd
		rm -f /var/run/ppconversd.pid
	fi
		ln -s /usr/local/etc/conversd/ppconversd.pid /var/run/ppconversd.pid
	touch /var/lock/subsys/ppconversd
	echo
	;;
  stop)
	if [ -L /var/run/ppconversd.pid ]
	then
		echo -n "Stopping ppconversd service: "
		killproc ppconversd
		rm -f /var/run/ppconversd.pid
		echo
	fi
	rm -f /var/lock/subsys/ppconversd 
	;;
  status)
	status ppconversd
	;;
  *)
	echo "Usage: $0 {start|stop|status}"
	exit 1
	;;
esac

exit 0
