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
    t2=$toptmp/python2out
    t3=$toptmp/python3out
    for i in *.py;do python $i ;done > $t2
    for i in *.py;do python3 $i ;done > $t3

    if ! cmp $t2 $t3 ; then 
        echo "Python2 and Python 3 outputs differ: $t2 $t3"
    fi
    for i in *.py;do xrun python $i ;done
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
