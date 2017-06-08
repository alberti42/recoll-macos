#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
    echo "Looking for content in HTML: should get 0 results"
    recollq '"Unity Lens""' dir:excludehtml
    echo "Looking for content in PDF: should get 1 result"
    recollq INFOMA dir:excludehtml
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
