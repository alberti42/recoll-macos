#!/bin/sh

if test ! -f shared.sh ; then
    echo must be run in the top test directory
    exit 1
fi

. shared.sh

# Yes, we could/should use the $toptmp from shared.sh here, but what if
# this is unset ?
toptmp=${TMPDIR:-/tmp}/recolltsttmp
if test -d "$toptmp" ; then
   rm -rf $toptmp/*
else
    mkdir $toptmp || fatal cant create temp dir $toptmp
fi

dirs=`ls -F | grep / | grep -v CVS`

for dir in $dirs ; do
    cd $dir && echo $dir
    sh `basename $dir`.sh
    cd ..
done
