#!/bin/sh

# Updating the index with Python and misc Python exercises

thisdir=`dirname $0`
topdir=$thisdir/..
. $topdir/shared.sh

initvariables $0

xrun()
{
    echo $*
    $*
}

(
    export RECOLL_TESTDATA=$tstdata
    export AUTHORNAME=somestrangeauthorname$$
    python3 updates.py
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
