#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
    # File names for files with skipped suffixes should be indexed,
    # and file names only (except for the .md5 one because it's in
    # noContentSuffixes-)
    recollq -S mtime Badsufffilename 

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
