#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# We need an UTF-8 locale here for recollq arg transcoding
LANG=en_US.UTF-8
export LANG

# The following tests have multiple results and the order keeps
# changing (relevance changes). Use sort to keep it quiet

(

recollq 'keyword:ckjtsthuniique まず' |sort
recollq 'keyword:ckjtsthuniique ます' |sort

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
