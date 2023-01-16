#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url '"Hallo friend"' date:2011-08-25
recollq -S url '"I like the G+ post you gave a"' date:2014-09-22
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
