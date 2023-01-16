#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

# Note: file -i returns application/octet-stream for
# somefile.info-1. xdg-mime query filetype returns text/plain. Recoll
# 1.22 and later return several results for the following query, one
# for the info file, and others in the somefile.info-xx files, as
# text/plain files. Previous versions only returned the info file,
# which was better, but I'm not sure that we can do something about
# it.
# It seems that later versions of xdg-mime return octet-stream so all back
# to normal
# Nope, not always: added mime type clause to fix. Using text/html
# because the result document is text/html|gnuinfo

(
    recollq '"GPGME is compiled with largefile support by default"' \
            mime:text/html
)  2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
