#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq Outmail_uniqueTerm          > $mystdout 2> $mystderr
recollq '"St Pierre en Chartreuse"' >> $mystdout 2>> $mystderr
recollq HtmlAttachment_uniqueTerm   >> $mystdout 2>> $mystderr
recollq '"Dear Corporate Administrator"' >> $mystdout 2>> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
