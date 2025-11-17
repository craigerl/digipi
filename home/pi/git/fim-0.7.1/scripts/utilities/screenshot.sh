#!/bin/sh
#
# $Id: screenshot.sh 213 2009-02-21 01:15:08Z dezperado $
# this script invokes fim to perform a screenshot of itself
#
# needs: fbgrab, convert, aa-enabled fim

FIM=src/fim

BFCMD="alias \"toggleVerbosity\" \"_display_console=1-_display_console;i:fresh=1;\";
toggleVerbosity;
load;
echo \"we type \\\"help\\\":\";
help;
echo \"we type \\\"commands\\\":\";
commands;
redisplay;
split;
window 'down';
vsplit;
auto_width_scale;
auto_width_scale;
display;
redisplay;
window 'right';
auto_height_scale;
rotate '34';
redisplay;
"

EFCMD='!"fbgrab fim-shot-aa.png";quit;'
$FIM -o aa -c "$BFCMD$EFCMD" media/*png
EFCMD='!"fbgrab fim-shot-fb.png";quit;'
$FIM -v    -c "$BFCMD$EFCMD" media/*png

# let's see
#$FIM fim-shot-fb.png fim-shot-aa.png 
# fim-shot-fb.png
SZ='400x300'
CA="-depth 3 -resize $SZ"

convert fim-shot-aa.png $CA fim-shot-aa-mini.png 
convert fim-shot-fb.png $CA fim-shot-fb-mini.png 

$FIM fim-shot-aa-mini.png fim-shot-fb-mini.png 

