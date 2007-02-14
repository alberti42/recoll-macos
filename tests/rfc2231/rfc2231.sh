#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# These one are name/filenamefor an inline part, no result
recollq EnerveUniqueNameTerm > $mystdout 2> $mystderr
recollq EnerveUniqueFileNameTerm >> $mystdout 2>> $mystderr
# This one for actual attachment
recollq EpatantUniquenameterm      >> $mystdout 2>> $mystderr
recollq EpatantUniquefilenameterm      >> $mystdout 2>> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
