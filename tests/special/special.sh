#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq uppercase_uniqueterm  > $mystdout 2> $mystderr
# The term is the lowercase utf8 version of the term in casefolding.txt
recollq 'àstrangewordþ'         >> $mystdout 2>> $mystderr
recollq '"Modernite/efficience/pertinence"' >> $mystdout 2>> $mystderr
recollq  dom.popup_allowed_events    >> $mystdout 2>> $mystderr
recollq  toto@jean-23.fr   >> $mystdout 2>> $mystderr


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
