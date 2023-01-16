#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url '"Akkusativ Nein Nein Regeln zur Bestimmung"' 
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
