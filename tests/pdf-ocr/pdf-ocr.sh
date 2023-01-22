#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    # This is from an image pdf. Only works if OCR is set up
    recollq '"bubbleupnp server to simulate openhome"'
    
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
