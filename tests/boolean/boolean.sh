#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq 'boolean_uniqueterm One OR Two -Three' 

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
