#!/bin/sh

# tests font functionality in fim
# this script shall work whatever its configure options (even if fim was compiled 'dumb')

f=${FIM_EXE-src/fim} # for e.g. Debian's post-installation tests
if test x != "$top_srcdir"; then true ; else top_srcdir=./ ; fi
ff="$top_srcdir/media/fim.png media/icon_smile.gif"
fa="-b -c quit $ff "
fv="-V "
fq="$FIMNORCOPTS"

fail()
{
	echo "[!] $@";
	exit -1;
}

resign()
{
	echo "[~] $@";
	exit 0;
}
succeed()
{
	echo "[*] $@";
	exit 0;
}

which grep || fail "we don't go anywhere without grep in our pocket"
g="`which grep` -i"

export FBFONT=''
e='s/^.*://g'

POD='\(sdl\|fb\|imlib2\)'
$f $fv 2>&1 | $g -i 'supported output devices.*:' | $g "$POD"|| resign "missing a pixel oriented driver (as one of $POD)"
$f $fv 2>&1 | $g 'supported file formats:'   | $g '\(png\|gif\)' | resign "missing adequate file format support"

if test x"$SSH_TTY" = x"" ; then
export FBFONT=/dev/null
if $f $fq $fa -o dumb ; then fail "$f $fa does not fail as it should on wrong font file" ; 
else echo "$f $fq $fa correctly recognizes an invalid FBFONT variable and exits" ; fi
else echo "Skipping FBFONT=/dev/null because seems we're running under ssh" ; fi

export FBFONT=$top_srcdir/var/fonts/Lat15-Terminus16.psf
if ! $f $fq $fa -o dumb ; then fail "$f $fq $fa fails, but it should not, as a correct font was provided" ; 
else echo "$f $fq $fa correctly recognizes a valid font file" ; fi

succeed "Font environment variables check PASSED"
exit 0

