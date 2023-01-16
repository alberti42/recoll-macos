#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  recollq '"content 4 9" OR "welcome to FreeBSD" OR zip_uniique'
  recollq ZIPPEDMAILDIR_UNIQUEXXX

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
