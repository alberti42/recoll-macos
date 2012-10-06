#!/bin/sh
# Case and diacritics sensitivity testing
# We use a separate configuration for this.
# This file is encoded in UTF-8: accentué

LANG=en_US.UTF-8
export LANG

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

RECOLL_CONFDIR=$topdir/casediac
export RECOLL_CONFDIR

cat > $RECOLL_CONFDIR/recoll.conf <<EOF
loglevel = 6
logfilename = /tmp/logrcltst

daemloglevel = 6
daemlogfilename = /tmp/rclmontrace

indexStripChars = 0

topdirs = $tstdata/casediac
indexstemminglanguages = french

EOF

recollindex -z

(
set -x
# Diac-sensitive search for existing term
recollq -s french éviter
# Succeeds because of default autodiacsens = false
recollq -s french èviter
# Diac-sensitive searches for non-existing term
recollq -s french '"èviter"D'
recollq -s french '"eviter"D'
# Diac-unsensitive search
recollq -s french eviter
# Search for stem-existing term (has to be unsensitive)
recollq -s french evite
# Search for stem-existing term with disabled stemming
recollq -s french Evite
# Case-sensitive, existing and non existing
recollq -s french MAJUSCULESXX
recollq -s french MAjUSCULESXX
# Case-unsensitive
recollq -s french majusculesxx
# Case-unsensitive
recollq -s french Majusculesxx
# Case sensitive, term does not exist
recollq -s french '"Majusculesxx"C'

)  2>&1 | egrep -v '^Recoll query: ' > $mystdout

diff -u -w ${myname}.txt $mystdout > $mydiffs 2>&1
checkresult

