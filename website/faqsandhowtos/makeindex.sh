#!/bin/sh
WIDX=WikiIndex.txt

echo "== Recoll Wiki file index" > $WIDX
for f in *.txt; do
 if test "$f" = $WIDX ; then continue; fi
 h="`basename $f .txt`.html"
 title=`head -1 "$f" | sed -e 's/=//g' -e 's/^ *//' -e 's/ *$//' -e 's///g'`
 echo 'link:'$h'['$title']' >> $WIDX
 echo >> $WIDX
done

exit 0
# Check and display what files are in the index but not in the contents table:

grep \| FaqsAndHowTos.txt | awk -F\| '{print $1}'  | sed -e 's/\* \[\[//' -e 's/.wiki//' |sort > ctfiles.tmp
grep '\[\[' WikiIndex.txt | awk -F\| '{print $1}'  | sed -e 's/\[\[//' -e 's/.wiki//' -e 's/.md//' | sort > ixfiles.tmp
echo 'diff ContentFiles  IndexFiles:'
diff ctfiles.tmp ixfiles.tmp
rm ctfiles.tmp ixfiles.tmp
