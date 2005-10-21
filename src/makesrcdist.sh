#!/bin/sh
#set -x
# A shell-script to make a recoll distribution:

TAR=/usr/bin/tar
 
targetdir=${targetdir-/tmp}
dotag=${dotag-yes}

if test ! -d qtgui;then
    echo "Should be executed in the master recoll directory"
    exit 1
fi

version=`cat VERSION`
versionforcvs=`echo $version | sed -e 's/\./_/g'`

topdir=$targetdir/recoll-$version
if test ! -d $topdir ; then
    mkdir $topdir || exit 1
else 
    echo "Removing everything under $topdir Ok ? (y/n)"
    read rep 
    if test $rep = 'y';then
    	rm -rf $topdir/*
    fi
fi

chmod +w README INSTALL
cat <<EOF > README

A more complete version of this document can be found at http://www.recoll.org


EOF
cat <<EOF > INSTALL

A more complete version of this document can be found at http://www.recoll.org


EOF

links -dump ~/projets/pagepers/recoll/index.html >> README
links -dump ~/projets/pagepers/recoll/credits.html >> README
links -dump ~/projets/pagepers/recoll/usermanual.html >> README
links -dump ~/projets/pagepers/recoll/installation.html >> INSTALL
cvs commit -m '' README INSTALL

# Clean up this dir and copy the dist-specific files 
make clean
yes | clean.O
$TAR chfX - excludefile .  | (cd $topdir;$TAR xvf -)

CVSTAG="RECOLL-$versionforcvs"
[ $dotag = "yes" ] && cvs tag -F $CVSTAG .

out=recoll-$version.tar.gz
(cd $targetdir ; \
    $TAR chf - recoll-$version | \
    	gzip > $out)
echo "$targetdir/$out created"
