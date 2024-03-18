#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Should find the file where its unaccented and the other and also an instance
# in misc.zip
(
    recollq -S url Anemometre 
    recollq -S url embed_stylesheet filename:'docutils*'

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
