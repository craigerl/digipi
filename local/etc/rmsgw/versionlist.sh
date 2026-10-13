#!/bin/bash
#
# versionlist.sh
#
# Interrogate Winlink web services
# requires curl & jq
#
# jq JSON parser is in these Debian distros:
#    wheezy-backports, jessie & stretch
# If you need to download it get it from here:
# http://stedolan.github.io/jq/download/
#

scriptname="`basename $0`"

TMPDIR="$HOME/tmp/rmsgw"
RMS_VERSION_FILE_RAW="$TMPDIR/rmsgwver.json"
RMS_VERSION_FILE_PARSE="$TMPDIR/rmsgwver_list.txt"
RMS_VERSION_FILE_OUT="$TMPDIR/rmsgwver.txt"
PKG_REQUIRE="jq curl"
# Intialize default version file refresh interval in hours
REFRESH_INTERVAL=10

DEBUG=

# controls when to call winlink web service api
do_it_flag=0

# Define a single white space for column formating
singlewhitespace=" "


## ============ functions ============

function dbgecho { if [ ! -z "$DEBUG" ] ; then echo "$*"; fi }
#
# Display program help info
#
usage () {
	(
	echo "Usage: $scriptname [-c][-d][-h][-t][-T]"
        echo "    -c switch to turn on displaying call sign list"
        echo "    -d switch to turn on verbose debug display"
        echo "    -t <int> Set refresh interval in hours. default: $REFRESH_INTERVAL hours"
        echo "    -T switch to turn off making Winlink service call."
        echo "    -h display this message."
        echo

	echo " exiting ..."
	) 1>&2
	exit 1
}

# is_pkg_installed

function is_pkg_installed() {
   return $(dpkg-query -W -f='${Status}' $1 2>/dev/null | grep -c "ok installed")
}

#
# Pad string with spaces
#
# arg1 = string
# arg2 = length to pad
#
format_space () {
   local whitespace=" "
   strarg="$1"
   lenarg="$2"
   strlen=${#strarg}
   whitelen=$(( lenarg-strlen ))
   for (( i=0; i<whitelen; i++ )) ; do
       whitespace+="$singlewhitespace"
    done;
# return string of whitespace(s)
    echo -n "$whitespace"
}

#
## =============== main ===============
#
# Initial TEST ONLY switch
TEST_ONLY="false"
COUNT_ONLY="true"

# Check for any command line arguments
# Command line args are passed with a dash & single letter
#  See usage function

while [[ $# -gt 0 ]] ; do

    key="$1"
    case $key in
        -c)
            dbgecho "Turn on displaying call sign list"
            COUNT_ONLY="false"
        ;;

        -d)
            echo "Turning on debug"
            DEBUG=1
        ;;
        -t)
            REFRESH_INTERVAL="$2"
            shift  #past value
            # Verify argument is an integer
            re='^[0-9]+$'
            if ! [[ $REFRESH_INTERVAL =~ $re ]] ; then
                echo "Error setting refresh interval: $REFRESH_INTERVAL not an integer"
                exit 1
            fi
            dbgecho "Set time in hours of last Winlink Service call to $REFRESH_INTERVAL hours."
        ;;
        -T)
            dbgecho "Turn off making Winlink Service call regardless of refresh_interval"
            TEST_ONLY="true"
        ;;
        -h)
            usage
            exit 0
        ;;
        *)
            echo "Undefined argument: $key"
            usage
            exit 1
        ;;
    esac
    shift # past argument or value
done

# Check for running as root
if [[ $EUID != 0 ]] ; then
    echo "Must be root to run this script"
    exit 1
fi

# check if required packages are installed
dbgecho "Check packages: $PKG_REQUIRE"
needs_pkg=false

for pkg_name in `echo ${PKG_REQUIRE}` ; do

   is_pkg_installed $pkg_name
   if [ $? -eq 0 ] ; then
      echo "$scriptname: Will Install $pkg_name program"
      needs_pkg=true
      break
   fi
done

if [ "$needs_pkg" = "true" ] ; then

   # Check for running as root
   if [[ $EUID != 0 ]] ; then
      echo "Must be root to install packages"
      sudo apt-get install -y -q $PKG_REQUIRE
   else
      apt-get install -y -q $PKG_REQUIRE
      if [ "$?" -ne 0 ] ; then
         echo "Required package install failed. Please try this command manually:"
         echo "apt-get install -y $PKG_REQUIRE"
         exit 1
      fi
   fi
fi

# Test if temporary directory exists
if [ ! -d "$TMPDIR" ] ; then
   echo "Directory: $TMPDIR does not exist, making ..."
   mkdir -p "$TMPDIR"
else
   dbgecho "Directory: $TMPDIR already exists"
fi

#  Test if temporary version file already exists and is not empty
if [ -s "$RMS_VERSION_FILE_RAW" ] ; then
    # Version file exists & is not empty
    # Determine how old the tmp file is

    current_epoch=$(date "+%s")
    file_epoch=$(stat -c %Y $RMS_VERSION_FILE_RAW)
    elapsed_time=$((current_epoch - file_epoch))
    elapsed_hours=$((elapsed_time / 3600))

    dbgecho "RMS Gateway Version file is: $elapsed_hours hours $((($elapsed_time % 3600)/60)) minute(s), $((elapsed_time % 60)) seconds old"

    # Only refresh the version file every day or so
    if ((elapsed_hours >= REFRESH_INTERVAL)) ; then
        echo "Refreshing version file: elapsed: $elapsed_hours hours, check interval: $REFRESH_INTERVAL hours"
        do_it_flag=1
    else
        dbgecho "Will NOT refresh version file: elapsed: $elapsed_hours hours, check interval: $REFRESH_INTERVAL hours"
    fi
else # Do this, if version file does NOT exist
    dbgecho "File $RMS_VERSION_FILE_RAW does not exist, running winlink api"
    do_it_flag=1
    elapsed_time=0
    elapsed_hours=0

fi # END Test if temporary version file already exists and not empty

# Check if refresh interval has passed
if [ $do_it_flag -ne 0 ]; then
    # Get the version information from the winlink server
    dbgecho "Refreshing version file from Winlink, with refresh interval: $REFRESH_INTERVAL hours"

    #  curl -s http://server.winlink.org:8085"/json/reply/GatewayProximity?GridSquare=$grid_square&MaxDistance=$max_distance" > $RMS__VERSION_RAW
    if [ "$TEST_ONLY" = "false" ] ; then
        python ./getversionlist.py > $RMS_VERSION_FILE_RAW
    else
        dbgecho "TEST_ONLY = true, otherwise would have called getversionlist.py, version list file NOT refreshed."
    fi
else
    # Display the information in the previously created version file
    echo "Using existing version file, with refresh interval: $REFRESH_INTERVAL hours"
    echo
fi

# Parse the JSON file
cat $RMS_VERSION_FILE_RAW | jq '.VersionList[] | {Callsign, Version, Timestamp}' > $RMS_VERSION_FILE_PARSE

# Print the table header
# echo "  Callsign        Version"

# iterate through the JSON parsed file
while read line ; do

  xcallsign=$(echo $line | grep -i "callsign")
  # If callsign variable exists, echo line to console
  if [ -n "$xcallsign" ] ; then
    callsign=$(echo $xcallsign | cut -d ':' -f2)

#    format_space $callsign 15

    # remove any spaces
    callsign=$(echo "$callsign" | tr -d ' ')
    # remove trailing comma
    callsign="${callsign%,}"
#    callsign=$(echo -n "${callsign//[[:space:]]/}")

    # remove both double quotes
    callsign="${callsign#\"}"
    callsign="${callsign%\"}"

    continue
  fi

  xversion=$(echo $line | grep -i "Version")
  if [ -n "$xversion" ] ; then
    version=$(echo $xversion | cut -d ':' -f2)
    # remove any spaces
    version=$(echo "$version" | tr -d ' ')
    # get rid of trailing comma
    version="${version%,}"

    # remove both double quotes
    version="${version#\"}"
    version="${version%\"}"

    continue
  fi

  xtimestamp=$(echo $line | grep -i "Timestamp")
  if [ -n "$xtimestamp" ] ; then
    timestamp=$(echo $xtimestamp | cut -d ':' -f2)
    # remove any spaces
    timestamp=$(echo "$timestamp" | tr -d ' ')
    # get rid of trailing comma
    timestamp="${timestamp%,}"
#    echo "Dist: $distance"

    # remove both double quotes
    timestamp="${timestamp#\"/Date(}"
    timestamp="${timestamp%)/\"}"

    # git rid of last 3 digits (milliseconds)
    convert_time=${timestamp:0:10}
    timestr="$(date -d @$convert_time)"
#    echo  "$callsign$(format_space $callsign 13) $version $(format_space $version 9) $convert_time $timestr"
    echo  "$callsign$(format_space $callsign 13) $version $(format_space $version 9) $timestr"

  fi

done < $RMS_VERSION_FILE_PARSE > $RMS_VERSION_FILE_OUT

if [ "$COUNT_ONLY" = "false" ] ; then
    sort -k 2 --numeric-sort $RMS_VERSION_FILE_OUT
    #sort -k 3 --numeric-sort $RMS_VERSION_FILE_OUT
    echo
fi

echo "Below rev: $(grep -c "2\.4\." $RMS_VERSION_FILE_OUT), Current: $(grep -c "2\.5\." $RMS_VERSION_FILE_OUT), Total: $(wc -l $RMS_VERSION_FILE_OUT | cut -d ' ' -f1) at $(date "+%b %_d %T %Z %Y")"
echo "RMS GW Version file is: $elapsed_hours hours $((($elapsed_time % 3600)/60)) minute(s), $((elapsed_time % 60)) seconds old"

exit 0
