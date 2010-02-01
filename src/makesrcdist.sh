#!/bin/sh
# @(#$Id: makesrcdist.sh,v 1.16 2008-11-21 16:43:42 dockes Exp $  (C) 2005 J.F.Dockes
# A shell-script to make a recoll source distribution

fatal() {
    echo $*
    exit 1
}
tagtopsvn() {
    (cd ..; svn copy -m "Release $version tagged" . $SVNREPOS/tags/$1) \
    	|| fatal tag failed
}
tagtophg() {
    hg tag -m "Release $version tagged" $1 || fatal tag failed
}
tagexists() {
    hg tags | cut -d' ' -f 1 | egrep '^'$1'$'
}
tagtop() {
    tagtophg $*
}

#set -x

TAR=/usr/bin/tar

#VCCMD=svn
#SVNREPOS=svn+ssh://y/home/subversion/recoll/

VCCMD=hg

if test ! -d qtgui;then
    echo "Should be executed in the master recoll directory"
    exit 1
fi

version=`cat VERSION`
versionforcvs=`echo $version | sed -e 's/\./_/g'`
TAG="RECOLL_$versionforcvs"

echo Creating version $versionforcvs
sleep 2
tagexists $TAG  && fatal "Tag $TAG already exists"

editedfiles=`$VCCMD status . | egrep -v '^\?'`
if test ! -z "$editedfiles"; then
	fatal  "Edited files exist: " $editedfiles
fi

targetdir=${targetdir-/tmp}
dotag=${dotag-yes}

case $version in
*.*.*) releasename=recoll-$version;;
*) releasename=betarecoll-$version;;
esac

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

################################### Documentation
###### Html doc
RECOLLDOC=${RECOLLDOC:=doc/user}
(cd $RECOLLDOC;make) || exit 1

###### Text Doc
chmod +w README INSTALL
cat <<EOF > README

More documentation can be found in the doc/ directory or at http://www.recoll.org


EOF
cat <<EOF > INSTALL

More documentation can be found in the doc/ directory or at http://www.recoll.org


EOF

echo "Dumping html documentation to text files"
links -dump ${RECOLLDOC}/usermanual.html >> README

links -dump ${RECOLLDOC}/rcl.install.html >> INSTALL
links -dump ${RECOLLDOC}/rcl.install.external.html >> INSTALL
links -dump ${RECOLLDOC}/rcl.install.building.html >> INSTALL
links -dump ${RECOLLDOC}/rcl.install.config.html >> INSTALL

$VCCMD commit -m "release $version" README INSTALL

# Clean up this dir and copy the dist-specific files 
make distclean
yes | clean.O
rm -f lib/*.dep
# Possibly clean up the cmake stuff
(cd kde/kioslave/recoll/ || exit 1
rm -rf CMakeCache.txt CMakeFiles/ CMakeTmp/ CPack* CTestTestfile.cmake cmake_* *automoc* lib)

$TAR chfX - excludefile .  | (cd $topdir;$TAR xf -)

# Fix the single/multiple page link in the header (we dont deliver the
# multi-page version and the file name is wrong anyway
sed -e '/\.\/index\.html/d' -e '/\.\/book\.html/d' \
    < $topdir/doc/user/usermanual.html > $topdir/doc/user/u1.html
diff $topdir/doc/user/u1.html $topdir/doc/user/usermanual.html
mv -f $topdir/doc/user/u1.html $topdir/doc/user/usermanual.html

# Can't now put ./Makefile in excludefile, gets ignored everywhere. So delete
# the top Makefile here (its' output by configure on the target system):
rm -f $topdir/Makefile

out=$releasename.tar.gz
(cd $targetdir ; \
    $TAR chf - $releasename | \
    	gzip > $out)
echo "$targetdir/$out created"

# Check manifest against current reference
tar tzf $targetdir/$out | sort | cut -d / -f 2- | \
    diff mk/manifest.txt - || fatal "Please fix file list"

# We tag .. as there is the 'packaging/' directory in there
[ $dotag = "yes" ] && tagtop $TAG
