#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url '"Evenements et programme 2006"' 
recollq -S url 'pcx11 manuel de programmation iamactuallyanrtf'
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
