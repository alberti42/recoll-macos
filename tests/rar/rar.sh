#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  recollq -q dir:rar '"Are you sure you want to delete this sheet"' 

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
