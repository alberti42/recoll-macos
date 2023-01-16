#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq author:gnumericAuthor 
    recollq gnumerictext
    recollq gnumericcommentaire
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
