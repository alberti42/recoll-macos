#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq '"Le pire mois de mai à Paris depuis 2012"'
    
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
