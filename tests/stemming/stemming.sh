#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq stemmingunique floor
recollq stemmingunique Floor

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
