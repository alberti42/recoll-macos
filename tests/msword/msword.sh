#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq '"Evenements et programme 2006"' 
recollq 'pcx11 manuel de programmation iamactuallyanrtf'
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
