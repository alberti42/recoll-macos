#!/bin/sh

# Test extended attributes indexing. This should work both with
# "detectxattronly" set or unset in the config, but should be run with
# the variable set, because we test its function by exploiting a bug
# (see comments further)
#
# We use the RECOLL_CONFTOP variable to add our own fields configuration

thisdir=`dirname $0`
topdir=$thisdir/..
. $topdir/shared.sh

initvariables $0

RECOLL_CONFTOP=$thisdir
export RECOLL_CONFTOP

xrun()
{
    echo $*
    $*
}

tstfile=${tstdata}/xattrs/tstxattrs.txt
rm -f $tstfile

(
    # Create the file with an extended attribute, index, and query it
    # by content and field
    echo xattruniqueinfile > $tstfile
    xrun pxattr -n myattr -v xattrunique1 $tstfile
    xrun recollindex -Zi $tstfile
    echo "1 result expected"
    xrun recollq xattruniqueinfile
    echo "1 result expected"
    xrun recollq myattr:xattrunique1 

    sleep 1

    # Change the value for the field, check that the old value is gone
    # and the new works
    xrun pxattr -n myattr -v xattrunique2 $tstfile
    xrun recollindex -i $tstfile
    echo "1 result expected"
    xrun recollq xattruniqueinfile
    echo "0 result expected:"
    xrun recollq myattr:xattrunique1 
    echo "1 result expected:"
    xrun recollq myattr:xattrunique2

    # Change the contents then the xattr. With xattronly set, recoll
    # should miss the contents change and index only the xattr. That's
    # a bug but we use it to check that pure xattr update indexing
    # works
    echo xattruniqueinfile1 > $tstfile
    sleep 2
    xrun pxattr -n myattr -v xattrunique3 $tstfile
    xrun recollindex -i $tstfile
    echo "1 result expected"
    xrun recollq xattruniqueinfile
    echo "0 result expected"
    xrun recollq xattruniqueinfile1
    echo "0 result expected:"
    xrun recollq myattr:xattrunique1 
    echo "0 result expected:"
    xrun recollq myattr:xattrunique2
    echo "1 result expected:"
    xrun recollq myattr:xattrunique3

    # Reset the index and check that the contents were seen all right
    xrun recollindex -Zi $tstfile
    echo "0 result expected"
    xrun recollq xattruniqueinfile
    echo "1 result expected"
    xrun recollq xattruniqueinfile1
    echo "0 result expected:"
    xrun recollq myattr:xattrunique2
    echo "1 result expected:"
    xrun recollq myattr:xattrunique3

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult
