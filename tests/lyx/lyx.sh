#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url 'Bienvenue Dans Univers De Lyx' 
recollq -S url 'Welcome To Lyx' 
recollq -S url 'LyX jol dokumentalt'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
