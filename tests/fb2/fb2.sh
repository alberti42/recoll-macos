#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url '"the Foreign Enlistment Act, and the Alabama case"' 
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
