#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq Outmail_uniqueTerm          2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq '"St Pierre en Chartreuse"' 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq HtmlAttachment_uniqueTerm   2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout
recollq '"Dear Corporate Administrator"' 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
