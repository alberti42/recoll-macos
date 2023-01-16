#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq '"Udi Manber and Sun Wu"' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

checkresult
