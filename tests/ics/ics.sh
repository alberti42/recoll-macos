#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq 'project xyz review meeting icsfile1_uniique' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

checkresult
