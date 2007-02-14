#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq 'Bienvenue Dans Univers De Lyx' > $mystdout 2> $mystderr
recollq 'Welcome To Lyx' >> $mystdout 2>> $mystderr
recollq 'Udvozli Ont A LyX' >> $mystdout 2>> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
