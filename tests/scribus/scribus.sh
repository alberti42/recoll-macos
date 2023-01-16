#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq -S url Scribus_sla_uniqueterm OR Chaturbhuja
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
