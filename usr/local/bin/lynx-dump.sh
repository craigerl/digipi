#!/bin/bash
#echo "total $#"
#echo "arg1 $1"
#echo "arg2 $2"
#echo "arg3 $3"

URL="$1"

if [ -z ${2} ]; then
    MAXLINES=60
else
    MAXLINES=${2}
fi

if [ $1 == "null" ]; then
    echo ""
    echo "Example, display the first 60(default) lines of wikipedia search for Ham Radio: "
    echo ""
    echo "       WEB   https://en.wikipedia.org/w/index.php?search=Ham_Radio   60 "
    echo ""
    exit 0
fi



URL="$1"

/usr/bin/lynx  -dump -accept_all_cookies "${URL}"   | head -n $MAXLINES
