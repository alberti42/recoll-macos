#!/bin/sh
# Build howto index page from howto subdirs

fatal()
{
    echo $*;exit 1
}
#set -x

test -f fraghead.html || \
     fatal repertoire courant pas un repertoire de construction

cat fraghead.html > index.html
subdirs=`ls -F | grep /`

for dir in $subdirs 
do
    echo processing $dir
    title=`grep '<h1>' $dir/index.html | sed -e 's/<h1>//' -e 's!</h1>!!'`
    test "$title" = "" && fatal No title line in $dir/index.html

    # Add title/label to list of articles
    echo "<li><a href=\"${dir}index.html\">$title</a></li>" >> index.html
done

cat fragend.html >> index.html
