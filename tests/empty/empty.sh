#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# check indexing of an empty directory name
(
    recollq -S url EmptyUniqueTerm

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
