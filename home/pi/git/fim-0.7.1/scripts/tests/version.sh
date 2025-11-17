#!/bin/sh

# tests the -V switch in fim
# TODO : should be integrated in the vim-like test suite.

f=${FIM_EXE-src/fim} # for e.g. Debian's post-installation tests

fail()
{
	echo "[!] $@";
	exit -1;
}
succeed()
{
	echo "[*] $@";
	exit 0;
}


which grep || fail "we don't go anywhere without grep in our pocket"
g="`which grep` -i"

# -V should return 0
$f -V 2>&1 > /dev/null || fail "$f -V returns an incorrect code"

e='s/^.*://g'

for s in 'supported output devices' 'supported file formats'
do
	$f -V 2>&1 | $g "$s" || fail "need the $s info message!"
	[ -z "`$f -V 2>&1 | $g \"$s\" | sed \"$e\"`" ] && fail "no $s ?"
done
succeed "Version string check based test PASSED"

exit 0

