#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# This is supposed to find an html attachment
recollq '"EMI is releasing albums for download"' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
