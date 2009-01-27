#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# We need an UTF-8 locale here for recollq arg transcoding
LANG=en_US.UTF-8
export LANG
recollq 'まず'  2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout
recollq 'ます'  2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
