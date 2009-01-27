#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
# The question is utf-8. As recollq will use the locale to translate the cmd
# line, need an utf-8 locale
export LANG=uk_UA.UTF-8
recollq 'внешнии' 2> $mystderr | 
	egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
