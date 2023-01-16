#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
recollq '"Philip K Dick 1962"'
recollq '"fatality. Miss Laura Chase, 25,"'
recollq '"Catherine trembled from head to foot"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
