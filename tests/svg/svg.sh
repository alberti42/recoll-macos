#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq -q title:svgtesttitle
    recollq -q '"Imbalance vector"' '"Correction vector"'
    recollq -q '"SR3000 Outer Ring"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
