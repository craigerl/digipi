#!/bin/bash
#
# Update python scripts & other files to be compatible with
#  Winlink Web Services V5
# If there is a command line argument then do a `diff` of files.

rmsgw_dir="/etc/rmsgw"
DIFF_ONLY=false

NEW_SERVICE_FILES="../etc/hosts winlinkservice.xml .version_info getsysop.py getchannel.py updatechannel.py  updatesysop.py updateversion.py getversionlist.py versionlist.sh"

# ===== function copy_files

function copy_files() {
    for filename in `echo ${NEW_SERVICE_FILES}` ; do
        if [ -e "${filename}.in" ] ; then
            echo "Copy autotool file $filename"
            cp -u "$filename.in" "$rmsgw_dir/$(basename $filename)"
        else
            cp -u "$filename" "$rmsgw_dir"
        fi
        if [ "$?" -ne 0 ] ; then
            echo "Error copying file: $filename"
        else
            chown rmsgw:rmsgw "$rmsgw_dir/$(basename $filename)"
        fi
    done
    cp -u ../etc/hosts $rmsgw_dir
}

# ===== function diff_files

function diff_files() {
    for filename in `echo ${NEW_SERVICE_FILES}` ; do
        if [ -e "${filename}.in" ] ; then
            echo "Diff autotool file $filename $(basename $filename)"
            diff "$filename.in" "$rmsgw_dir/$(basename $filename)"
        else
            diff "$filename" "$rmsgw_dir"
        fi
        if [ "$?" -ne 0 ] ; then
            echo "Files differ: $filename"
        fi
    done
}

# ===== main
echo "Update to V5 Winlink Web Service API"

# make sure we're running as root
if [[ $EUID != 0 ]] ; then
   echo "Must be root"
   exit 1
fi

# Check for any arguments
if (( $# != 0 )) ; then
   DIFF_ONLY=true
fi

if [ ! -d "$rmsgw_dir" ] ; then
    echo "Directory $rmsgw_dir does not exist ... exiting"
#    mkdir -p "$dest_dir"
    exit 1
fi

if [ -e "$rmsgw_dir/sysop.xml" ] ; then
    echo "Found file: $rmsgw_dir/sysop.xml"
    grep "Password" $rmsgw_dir/sysop.xml  > /dev/null 2>&1
    if [ $? -ne 0 ] ; then
        echo "Adding password to $rmsgw_dir/sysop.xml"
        sed -i -e '/<Callsign>/a\
    <Password></Password>' $rmsgw_dir/sysop.xml
    else
        echo "Password element found."
    fi
else
    echo "File: $rmsgw_dir/sysop.xml not found."
fi

if $DIFF_ONLY ; then
    diff_files
    action="diff"
else
    copy_files
    action="update"
fi
echo "RMS Gateway files & scripts $action completed."

exit 0
