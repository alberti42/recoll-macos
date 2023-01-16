#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
recollq DEPARTMENT NumberOfEmployees
recollq Within5Min WaterLevel KeptBelow
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
