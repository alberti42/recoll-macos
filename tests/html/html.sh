#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq QMapConstIterator > $mystdout 2> $mystderr
recollq Qtextedit Widget Provides Powerful Single-Page >> $mystdout 2>> $mystderr
recollq '"This is the Mysql reference manual"'  >> $mystdout 2>> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
