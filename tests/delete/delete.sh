#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Create file to be deleted, index, query. Note: the 'delete' directory must not be empty,
# else the dismounted media test will trigger in the second indexer and the subtree will
# be marked existing (no purge).
(
    echo "DeletedFileUnique" > ${tstdata}/delete/tobedeleted.txt
    recollindex
    recollq DeletedFileUnique 

    # Delete file and query again
    rm ${tstdata}/delete/tobedeleted.txt
    recollindex
    recollq DeletedFileUnique

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
