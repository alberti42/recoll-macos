#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq 'Bienvenue Dans Univers De Lyx' 
recollq 'Welcome To Lyx' 
recollq 'LyX jol dokumentalt'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
