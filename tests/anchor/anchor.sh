#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
for q in \
'"^anchortermeaudebut"' \
'"^ anchortermeunpeuplusloin"' \
'"^anchortermeunpeuplusloin"o30' \
'"^  anchortermeunpeuplusloin"o30' \
'"anchortermenullepart"' \
'"^anchortermenullepart"' \
'"anchortermenullepart $"' \
'"anchortermeunpeumoinsloin$"o30' \
'"anchortermeunpeumoinsloin$"' \
'"anchortermealafin$"' \
'title:"^anchortitlebegin"' \
'title:"^anchortitleend"' \
'title:"anchortitleend$"' \
; do 
    echo $q
    recollq -q $q
done

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
