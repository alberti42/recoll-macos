#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  # Only
  recollq filename:testdirfile dir:testrecoll/dir/d1 OR \
                               dir:testrecoll/dir/d2  -dir:d2

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
