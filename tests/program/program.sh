#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    # Suppress the mime type because it may be application/xxx or text/xxx
    recollq '"maps image file tags to xapian tags"' |  awk '{$1 = "";print $0}'
    recollq '"PMC = Performance Monitor Control MSR"' | awk '{$1 = "";print $0}'
    recollq shellscriptUUnique | awk '{$1 = "";print $0}'
    # Python: query.py
    recollq Xesam QueryException 
  
) 2> $mystderr | egrep -v 'query: ' > $mystdout

checkresult
