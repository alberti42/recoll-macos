#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    # skippedPaths:
    # shouldbeskipped.txt should be skipped because
    # skipped/reallyskipped/ is in skippedPaths, but the query gets 1
    # result because of rlyskipped/shouldnotbeskipped.txt
    # 1 res: shouldnotbeskipped.txt
    recollq ShouldbeSkippedUnique

    # skippedNames
    # recoll.ini is in the default skippedNames list, but should be the
    # result here because 'recoll.ini' is in the local config skippedNames-
    # 1 res: skipped/recoll.ini
    recollq recollrcUnique

    # skippedNames
    # Should be skipped because notinskippednames is in skippedNames+
    # 0 res for skipped/notinskippednames
    recollq -q notinskippedNamesUnique
    
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
