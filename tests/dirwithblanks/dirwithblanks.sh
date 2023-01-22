#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
    set -x
recollq DirWithBlanksUnique 
recollq DirWithBlanksUnique dir:\"$tstdataindir/dir with blanks\"
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
