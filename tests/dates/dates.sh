#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
# 2nd one result

(
# no results because of the negative clause
recollq date:/2009 -1 datetest
# one result: the 2008 file
recollq date:/2009    datetest
# one result: 2008 file
recollq date:2007-01-01/2010-02-09 datetest
# 2 results : files from 10 and 11 march 2010
recollq -S mtime date:2010-03-10/P10D datetest
# 0 result: no such term
recollq date:2010-03-10/P10D datetestnotermsmatch
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
