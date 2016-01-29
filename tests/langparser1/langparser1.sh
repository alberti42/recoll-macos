#!/bin/sh

# langparser actually test queries. We only test the language parser, the tested reference is the Xapian query.

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0
xrun()
{
    echo $*
    $*
}

(
    for Q in  \
    'A' \
    'A B' \
    'A AND B' \
    'A OR B' \
    'A OR B AND C' \
    'A AND B OR C' \
    '(A AND B) OR (C AND D)' \
    '(A OR B) AND (C OR D)' \
    '-the B' \
    'A -B' \
    'mime:text/plain' \
    'size>10k' \
    'date:3000-01-01' \
    'mime:text/plain A OR B mime:text/html' \
    'mime:text/plain A AND B mime:text/html' \
    'mime:text/plain mime:text/html (A B) ' \
    'mime:text/plain OR mime:text/html OR (A B) ' \
    'rclcat:media A' \
    'rclcat:media rclcat:message A' \
    'A size>10k' \
    'size>10k A' \
    'date:3000-01-01 A' \
    'A OR B date:3000-01-01' \
    'A OR B AND date:3000-01-01' \
    'title:A B' \
    'title:A -B' \
    'A -title:B' \
    ; do
        # The " $Q" is there to avoid issue with a query beginning with -
        # (recollq does not grok --)
        printf "%60s" "Query: $Q -> ";recollq -Q -q " $Q"
    done
)  2> $mystderr | egrep -v 'results|^Query setup took' > $mystdout

diff -w ${myname}.txt $mystdout > $mydiffs 2>&1

checkresult
