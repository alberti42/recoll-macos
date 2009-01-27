#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Should find the file where its unaccented and the other
recollq Anemometre 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
