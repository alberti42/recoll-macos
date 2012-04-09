#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq 'Bienvenue Dans Univers De Lyx' 
recollq 'Welcome To Lyx' 
recollq 'LyX rendkivul jol dokumentalt'
# cant use this one because o-diaeresis is in the swedish unacex
# recollq 'Udvozli Ont A LyX' 
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
