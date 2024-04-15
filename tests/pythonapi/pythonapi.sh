#!/bin/sh

# Test the Python API

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
    t3=$toptmp/python3out
    for i in *.py;do xrun python3 $i ;done
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
