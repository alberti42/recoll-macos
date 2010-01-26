#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

recollq '"nokia ovi suite" wmdrm "windows media player version 11"' \
2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

recollq '"pour superposer mixer des fichiers son"' \
2>> $mystderr | egrep -v '^Recoll query: ' >> $mystdout

recollq '"Django comes with a user authentication system"' \
2>> $mystderr | egrep -v '^Recoll query: ' >> $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
