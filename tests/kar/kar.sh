#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
export LC_ALL=en_US.UTF-8

(
    recollq -S mtime '"Ночь и тишина, данная на век"'
    recollq -S mtime '"Man it’s a hot one"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
