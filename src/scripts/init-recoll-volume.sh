#!/bin/sh


# Set up a mounted volume (e.g. external SSD, USB thumb drive...) for
# portable Recoll indexing.
#
# We take the root of the indexable part of the volume as parameter
# (possibly the mount point): $topdir. We create a configuration
# directory as $topdir/recoll-config, set the topdirs and
# orgidxconfdir recoll.conf parameters accordingly, and run
# recollindex

fatal()
{
    echo $*;exit 1
}
usage()
{
    fatal "Usage: init-recoll-volume.sh <top-directory-on-volume>"
}

test $# = 1 || usage
topdir=$1
test -d "$topdir" || fatal $topdir should be a directory

confdir="$topdir/recoll-config"
test ! -d "$confdir" || fatal $confdir should not exist

mkdir -p "$confdir"
cd "$topdir"
topdir=`pwd`
cd "$confdir"
confdir=`pwd`

(echo topdirs = '"'$topdir'"'; \
 echo orgidxconfdir = $topdir/recoll-config) > "$confdir/recoll.conf"

recollindex -c "$confdir"
