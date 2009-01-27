#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# These one are name/filenamefor an inline part, no result
recollq EnerveUniqueNameTerm 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

recollq EnerveUniqueFileNameTerm 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
# This one for actual attachment
recollq EpatantUniquenameterm      2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq EpatantUniquefilenameterm      2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
