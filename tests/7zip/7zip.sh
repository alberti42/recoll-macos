#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  recollq '7zr_unique'

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
