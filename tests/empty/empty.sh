#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# check indexing of an empty directory name
recollq EmptyUniqueTerm > $mystdout 2> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
