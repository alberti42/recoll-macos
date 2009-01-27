#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq QMapConstIterator 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq Qtextedit Widget Provides Powerful Single-Page 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq '"This is the Mysql reference manual"' 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
