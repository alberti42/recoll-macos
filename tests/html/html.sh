#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
recollq QMapConstIterator
recollq Qtextedit Widget Provides Powerful Single-Page 
recollq '"This is the Mysql reference manual"' 
recollq -a 'etonne badhtml' 
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
