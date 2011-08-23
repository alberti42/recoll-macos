#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  recollq -q sheetgen '"Are you sure you want to delete this sheet"' 

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
