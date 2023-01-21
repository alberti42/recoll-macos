#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  # Only
  recollq filename:testdirfile dir:dir/d1 OR dir:dir/d2  -dir:d2

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
