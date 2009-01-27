#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq stemmingunique floor 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq stemmingunique Floor 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
