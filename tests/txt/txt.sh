#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Should find the file where its unaccented and the other
recollq Anemometre > $mystdout 2> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
