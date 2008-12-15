#!/bin/sh

fatal()
{
    echo $*; exit 1
}
usage()
{
    fatal 'Usage: newdir nom'
}

test $# -gt 1 || usage

dir=`echo $* | sed -e 's/ /_/g' -e 's!/!_!g'`

echo dir: $dir

mkdir $dir || fatal mkdir failed

cp -i template.html $dir/index.html 

open -a emacs $dir/index.html
