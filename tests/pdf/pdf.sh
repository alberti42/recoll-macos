#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
    recollq '"sequences of moveto and lineto"' OR 'ANSIX3.4'
    # pdf xmp. Note that dc:identifier is aliased to url in the
    # defaults field file, and this can't be overruled afaics, so
    # url is prefixed for the dc:identifier search to work
    recollq dc:identifier:10.12345/sampledoi
    recollq 'pdf:Producer:"GPL Ghostscript 9.18"'
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
