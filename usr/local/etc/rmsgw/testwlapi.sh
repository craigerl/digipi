#!/bin/bash

# Lowest python version required to run the admin scripts
# This version ships with Debian jessie
lowest_pyver="2.7.9"

rmsgw_dir="/etc/rmsgw"
debuglog_dir="/root/tmp"
debuglog_file="debuglog.txt"
RMSLOG_FILE="/var/log/rms.debug"

# Function ver_lte, compare version numbers
function ver_lte() {
    [  "$1" = "`echo -e "$1\n$2" | sort -V | head -n1`" ]
}

# make sure we're running as root
if [[ $EUID != 0 ]] ; then
   echo "Must be root"
   exit 1
fi


if [ ! -d "$debuglog_dir" ] ; then
    echo "Directory $debuglog_dir does not exist ... creating"
    mkdir -p "$debuglog_dir"
fi

echo "Starting test at $(date) ..."

# Output following this is redirected to the debug log file
{

# Verify that the RMS Gateway logfile exists
if [ ! -e "$RMSLOG_FILE" ] ; then
    echo "Warning log file: $RMSLOG_FILE doe not exist, using syslog"
    RMSLOG_FILE="/var/log/syslog"
fi

# This start time matches log entry times
starttime=$(date "+%b %_d %H:%M:")

# Get time of last log entry in $RMSLOG_FILE
logdate=$(tail -1 $RMSLOG_FILE | cut -d':' -f 1-2)
echo "Start time: $(date), verify with logfile date format: $logdate"

# Get some version info
cat /etc/*release | grep "VERSION"
cat /etc/*version
cat /proc/version
# Not all Linux distros run GNU coreutils
sort --version

echo

current_pyver="$(python --version 2>&1)"
echo "Debug: pyver: $current_pyver"
if [ -z "$current_pyver" ] ; then
    echo "No python version string found."
else
    echo "Check Python ver #, current: $current_pyver, lowest: $lowest_pyver"
    ver_lte $current_pyver $lowest_pyver
    if [ "$?" -eq 0 ] ; then
        echo "Version check($?) less than required, current ver: $current_pyver"
    else
        echo "Version check($?) OK, current ver: $current_pyver"
    fi
fi

cd /etc/rmsgw

echo
echo "Log mask set to: $(grep -i "mask" /etc/rmsgw/gateway.conf)"

echo
echo "===== getchan ====="
sudo -u rmsgw ./getchannel.py -d
echo
echo "===== updateversion ====="
sudo -u rmsgw ./updateversion.py -d; echo $?
echo
echo "===== updatechannel ====="
sudo -u rmsgw ./updatechannel.py -d; echo $?
echo
echo "===== getsysop ====="
./getsysop.py -d; echo $?
echo
echo "===== updatesysop ====="

sysop_file="$rmsgw_dir/sysop.xml"
if [ -e "$sysop_file" ] ; then
   sudo -u rmsgw ./updatesysop.py -d
else
   echo "Error: file: $sysop_file does not exist"
fi
echo
echo "===== rms.debug log ====="
#tail -n 50 $RMSLOG_FILE
# List log file from when this script started
sed -n '/'"$logdate"'/,$p' $RMSLOG_FILE

echo
echo "===== channels.xml ====="
cat /etc/rmsgw/channels.xml
echo
echo "===== winlinkservice.xml ====="
cat /etc/rmsgw/winlinkservice.xml
echo
echo "===== sysop.xml ====="
cat /etc/rmsgw/sysop.xml
} > $debuglog_dir/$debuglog_file 2>&1

# Remove any passwords
sed -i "s/\('password': '\).*\(',\)/\1notyourpassword\2/" $debuglog_dir/$debuglog_file
sed -i "s|\(<Password>\)[^<>]*\(</Password>\)|\1notyourpassword\2|" $debuglog_dir/$debuglog_file
sed -i "s|\(<password>\)[^<>]*\(</password>\)|\1notyourpassword\2|" $debuglog_dir/$debuglog_file
sed -i "s|\(<ns0:password>\)[^<>]*\(</ns0:password>\)|\1notyourpassword\2|g" $debuglog_dir/$debuglog_file

echo "test finished at $(date) ..."
exit 0
