#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# We need an utf-8 locale for the 1st command to properly read its argument
export LANG=fr_FR.UTF-8

(
# Should succeed
recollq '"strippes: UNACEXååääöö"' 
# Should fail
recollq '"strippes: UNACEXaaaaoo"'
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
