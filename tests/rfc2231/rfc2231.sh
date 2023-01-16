#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
# These one are name/filenamefor an inline part, no result
recollq EnerveUniqueNameTerm

recollq EnerveUniqueFileNameTerm
# This one for actual attachment
recollq EpatantUniquenameterm
recollq EpatantUniquefilenameterm

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
