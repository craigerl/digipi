#!/bin/sh

# This sample scripts shows the use of fim for assisted batch scanning in a X free environment.

function die()  { echo " [!] : " $@ ; exit 1 ; }
function info() { echo " [*] : " $@ ; }
function askn() { local c ; c='' ; info $@ ; read c ; if test "$c" != "y" -a  "$c" != "Y"  ; then return -1 ; fi ; return  0 ; }
function asky() { local c ; c='' ; info $@ ; read c ; if test "$c" != "n" -a  "$c" != "N"  ; then return 0  ; fi ; return -1 ; }
function askexit() { asky "are you sure you want to exit [Y/n]?" || return 0 ; info "ok. exiting now..." ; exit ; }

[[ $1 != "" ]] && { info "this is a sample script using fim and scanimage...  invoke it without arguments please." ; exit ; }

shopt -s expand_aliases || die "no aliases ? you should update your bash shell.."
scanimage --version 2>&1 > /dev/null || die "no scanimage ?"

GEOMETRY=' -l0mm -t0mm -x210mm -y300mm'
LODEPTH='--depth 1'
HIDEPTH='--depth 32'
HIRES='--resolution 200'
LORES='--resolution 20'
FORMAT='--format=tiff'
FORMAT=
alias scan.a4='scanimage $GEOMETRY $HIDEPTH $HIRES $FORMAT'
alias scan.lo='scanimage $GEOMETRY $LODEPTH $LORES $FORMAT'
alias scan='scan.a4'

alias filter_bugged_scanimage_pnm="grep -v 'scanimage.*backend'"

DIR="${TMPDIR-/var/tmp}/fbscan-$$"
TF=$DIR/scan.fim.tmp

# we must have write permissions in that directory
mkdir -p $DIR	|| exit 1

# we want that directory clean after this script execution, no matter what happens
trap "rm -rf $DIR" EXIT

i=1;
FN=`printf "scan-%004d.pnm" "$i"`

while !  askn "do you to wish start scanning ? we begin with $FN [y/N]" ; do askexit ; done

for ((i=1;i<100;i+=0))
do
	info "scanning $FN..."
	scan.lo | filter_bugged_scanimage_pnm | fim -i -c 'bind "y" "return 0;";bind "n" "return -1;";status "press y to scan this image in high resolution. press n otherwise."'  && scan.hi > $FN
	if test "$?" = "0" ; then ((++i))  ; fi
	FN=`printf "scan-%004d.pnm" "$i"`
	while !  askn "do you wish to continue scanning ? we continue with $FN [y/N]" ; do askexit ; done
done

