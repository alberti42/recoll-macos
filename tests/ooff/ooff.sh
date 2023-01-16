#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(

recollq -S url -D OpenofficeWriter_uniqueterm 
recollq -S url -D SoffTabsUnique
recollq -S url -D FirstSlideUnique

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
