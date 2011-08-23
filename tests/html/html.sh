#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
(
recollq QMapConstIterator
recollq Qtextedit Widget Provides Powerful Single-Page 
recollq '"This is the Mysql reference manual"' 
# Tests that the charset spec is correctly recognised inside badhtml.html
recollq -a 'etonne badhtml' 
# Tests field extraction/storage and indexing
recollq -m -q "testfield:testfieldvalue" | egrep 'results|^text/html|^testfield ='

# more unaccenting tests
recollq -q 'effaranteUTF8HTML'
recollq -q 'effrayanteUTF8HTML'
recollq -q 'accentueesUTF8HTML'
recollq -q 'accentueesISOHTML'

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
