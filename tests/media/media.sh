#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq author:clash 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq title:lux title:aeterna 2> $mystderr |
        egrep -v '^Recoll query: ' >> $mystdout
recollq live at leeds the who 2> $mystderr |
        egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
