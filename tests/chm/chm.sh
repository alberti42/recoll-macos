#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq -S url '"nokia ovi suite" wmdrm "windows media player version 11"' 
recollq -S url '"pour superposer mixer des fichiers son"'
recollq -S url '"Django comes with a user authentication system"'
recollq -S url '"establishment of a project cost accounting system of ledgers"'
# xxxCriticalDrugTherapy has encoded internal urls.
recollq -S url '"Cocaine-induced ACS"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
