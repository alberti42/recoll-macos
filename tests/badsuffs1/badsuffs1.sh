#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# File names for files with skipped suffixes should be indexed
recollq Badsufffilename > $mystdout 2> $mystderr

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
