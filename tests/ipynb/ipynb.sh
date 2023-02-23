#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq -D -S url -q '"Aw, yeah! It'"'"'s even got"'
    recollq -D -S url -q '"def myfunc"' ext:ipynb
    recollq -D -S url -q '"#@title 素材打包"'
    recollq -D -S url -q '"2 informative features and 2 noise features"'
) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
