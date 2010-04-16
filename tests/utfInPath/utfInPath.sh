#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq -q '"simulating shock turbulence interactions"' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

recollq Utf8pathunique 2> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
