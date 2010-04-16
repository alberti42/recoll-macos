#!/bin/sh

if test ! -f shared.sh ; then
    echo must be run in the top test directory
    exit 1
fi

. shared.sh

makeindex() {
  echo "Zeroing Index" 
  rm -rf $RECOLL_CONFDIR/xapiandb $RECOLL_CONFDIR/aspdict.*.rws
  echo "Indexing" 
  recollindex -z
}

makeindex

# Yes, we could/should use the $toptmp from shared.sh here, but what if
# this is unset ?
toptmp=${TMPDIR:-/tmp}/recolltsttmp
if test -d "$toptmp" ; then
   rm -rf $toptmp/*
else
    mkdir $toptmp || fatal cant create temp dir $toptmp
fi

dirs=`ls -F | grep / | grep -v CVS`

echo
echo "Running query tests:"

for dir in $dirs ; do
    cd $dir && echo -n "$dir "
    sh `basename $dir`.sh
    cd ..
done

echo
