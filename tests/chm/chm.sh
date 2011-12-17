#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq '"nokia ovi suite" wmdrm "windows media player version 11"' 
recollq '"pour superposer mixer des fichiers son"'
recollq '"Django comes with a user authentication system"'
recollq '"establishment of a project cost accounting system of ledgers"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
