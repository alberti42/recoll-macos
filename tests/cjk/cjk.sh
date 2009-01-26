#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq まず  > $mystdout 2> $mystderr
recollq ます  > $mystdout 2> $mystderr


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
