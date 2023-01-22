#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(

recollq -S fmtime Utf8pathunique 

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
