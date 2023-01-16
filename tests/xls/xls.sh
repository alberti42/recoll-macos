#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq Excel_UniqueTerm 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

checkresult
