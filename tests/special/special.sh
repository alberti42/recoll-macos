#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Needs utf-8 locale
LANG=en_US.UTF-8
export LANG

recollq uppercase_uniqueterm 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
# The term is the lowercase utf8 version of the term in casefolding.txt
recollq 'àstrangewordþ'          2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq '"Modernite/efficience/pertinence"'  2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq  dom.popup_allowed_events     2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq  toto@jean-23.fr    2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
