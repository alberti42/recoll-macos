#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# We need an UTF-8 locale here for recollq arg transcoding
LANG=en_US.UTF-8
recollq 'まず'  > $mystdout 2> $mystderr
recollq 'ます'  >> $mystdout 2> $mystderr


diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
