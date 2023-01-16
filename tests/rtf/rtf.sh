#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq EQUK0401319A 

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
