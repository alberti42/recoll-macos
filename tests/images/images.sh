#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Image names 
recollq photovoeux > $mystdout 2> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
