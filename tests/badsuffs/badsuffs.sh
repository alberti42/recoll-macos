#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    # Data in text files with skipped suffixes should not be indexed,
    # except that .md5 is in noContentSuffixes- -> 1 result,
    # badsuffilename.md5
    recollq Badsuffixes_unique

    # .nosuff is added by noContentSuffixes+. No result for you
    # notreallybad.nosuff
    recollq nosuffUnique
    
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
