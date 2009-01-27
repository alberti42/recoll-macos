#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

tstfile=${tstdata}/delete/tobedeleted.txt

# Create file to be deleted, index, query
echo "DeletedFileUnique" > $tstfile
recollindex >> $mystderr 2>&1 
recollq DeletedFileUnique 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

# Delete file and query again
rm -f ${tstdata}/delete/tobedeleted.txt
recollindex  >> $mystderr 2>&1 
recollq DeletedFileUnique 2>> $mystderr | 
	egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
