#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq 'Bienvenue Dans Univers De Lyx' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq 'Welcome To Lyx' 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq 'Udvozli Ont A LyX' 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
