#!/bin/sh

thisdir=`dirname $0`
topdir=$thisdir/..
. $topdir/shared.sh

initvariables $0

(
echo "Should find idxtypes/idxt.txt only"
recollq idxtypesunique
echo

echo "Should find idxtypes/idxt.html only"
recollq excltypesunique

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
