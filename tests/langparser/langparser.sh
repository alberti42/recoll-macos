#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
xrun()
{
    echo $*
    $*
}

(
    echo "Expect 2 bu.txt leli.txt"
    recollq 'andorhuniique Beatles OR Lennon Live OR Unplugged' 
    echo "Expect 2 bu.txt leli.txt"
    recollq 'andorhuniique (Beatles OR Lennon) AND (Live OR Unplugged)' 
    echo "Expect 2 bu.txt leli.txt"
    recollq '(Beatles OR Lennon) Live OR Unplugged andorhuniique' 
    echo "Expect 2 bu.txt leli.txt"
    recollq 'Beatles OR Lennon (Live OR Unplugged) andorhuniique' 
    echo "Expect 1 bu.txt"
    recollq 'Beatles OR Lennon -Lennon (Live OR Unplugged) andorhuniique' 
    echo "Expect 1 leli.txt"
    recollq 'Beatles OR Lennon Lennon Live andorhuniique' 
    echo "Expect 1 leli.txt"
    recollq 'Beatles OR Lennon Live andorhuniique' 
    echo "Expect 1 lb.txt"
    recollq '(Beatles AND Live) OR (Beatles AND Lennon) andorhuniique' 
    echo "Expect 1 lb.txt"
    recollq '(Beatles Live) OR (Beatles AND Lennon) andorhuniique' 
    echo "Expect 1 leli.txt"
    recollq '(Beatles AND Live) OR (Lennon AND Unplugged OR Live) andorhuniique' 
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html Fields Test File"' 
    echo "Expect 0"
    recollq 'title:"Html Fields File"' 
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html Fields File"o' 
    echo "Expect 0"
    recollq 'title:"Html File Fields"' 
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p'
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p -nosuchtermexists'
    echo "Expect 0 htmlfield1.html"
    recollq 'title:"Html File Fields"p nosuchtermexists'
    echo "Expect 1 htmlfield1.html"
    recollq 'ThisIsTheFieldHtmlTestFile title:"Html File Fields"p -nosuchtermexists'
    echo "Expect 2 htmlfield.html htmlfield1.html"
    recollq 'Beatles OR ThisIsTheFieldHtmlTestFile title:"Html File Fields"p'
    echo "Expect 1 htmlfield1.html"
    recollq 'ThisIsTheFieldHtmlTestFile OR title:"Html File Fields"p'

    # Size tests. Note that the search code handles < and > as <= and
    # >= for now. So size>267 succeeds...
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p size=267'
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p size>= 267'
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p size <=267'
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p size <= 300'
    echo "Expect 1 htmlfield1.html"
    recollq 'title:"Html File Fields"p size >= 200'
    echo "Expect 0"
    recollq 'title:"Html File Fields"p size=268'
    echo "Expect 0"
    recollq 'title:"Html File Fields"p size>268'
    echo "Expect 0"
    recollq 'title:"Html File Fields"p size<266'

)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
