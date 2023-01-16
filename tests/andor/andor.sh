#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
xrun()
{
    echo $*
    $*
}

(
    recollq 'andorhuniique Beatles OR Lennon Live OR Unplugged' 
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
