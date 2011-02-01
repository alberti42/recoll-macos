#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
export LC_ALL=en_US.UTF-8

(
    recollq '"Ночь и тишина, данная на век"'
    recollq '"Man it’s a hot one"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
