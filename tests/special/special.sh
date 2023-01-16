#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Needs utf-8 locale
LANG=en_US.UTF-8
export LANG

(
recollq uppercase_uniqueterm
# The term is the lowercase utf8 version of the term in casefolding.txt
recollq 'àstrangewordþ'

recollq '"Modernite/efficience/pertinence"'
recollq  dom.popup_allowed_events
recollq  toto@jean-23.fr

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout


checkresult
