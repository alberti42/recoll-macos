#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
    recollq -q '"ceci est le resume 1"'
    recollq -q '"Managed 50 UNIX computers"'

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
