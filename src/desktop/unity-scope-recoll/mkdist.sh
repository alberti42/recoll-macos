#!/bin/sh

fatal() {
    echo $*
    exit 1
}
usage() {
    echo 'Usage: mkdist.sh '
    exit 1
}

VCCMD=hg
TAR=/usr/bin/tar
TAR=tar

VRECOLL=`cat ../../VERSION`
VLENS=`hg tip | egrep ^changeset: | awk '{print $2}' | awk -F: '{print $1}'`
VERSION=${VRECOLL}.${VLENS}
echo $VERSION

targetdir=${targetdir-/tmp}

checkmodified=${checkmodified-yes}

#editedfiles=`$VCCMD status . | egrep -v '^\?'`
if test "$checkmodified" = "yes" -a ! -z "$editedfiles"; then
  fatal  "Edited files exist: " $editedfiles
fi

releasename=recoll-scope-${VERSION}

topdir=$targetdir/$releasename
if test ! -d $topdir ; then
    mkdir $topdir || exit 1
else 
    echo "Removing everything under $topdir Ok ? (y/n)"
    read rep 
    if test $rep = 'y';then
    	rm -rf $topdir/*
    fi
fi

# Clean up this dir and copy the dist-specific files 
#make distclean
yes | clean.O

$TAR chfX - excludefile .  | (cd $topdir;$TAR xf -)

out=$releasename.tar.gz
(cd $targetdir ; \
    $TAR chf - $releasename | \
    	gzip > $out)
echo "$targetdir/$out created"
