#!/bin/sh

fatal()
{
    echo $*
    exit 1
}
usage()
{
    fatal make-recoll-dmg.sh
}

# Adjustable things
top=~/Recoll
# The possibly bogus version we have in paths (may be harcoded in the .pro)
qtpathversion=5.14.2
# Will probably need adjustment on M1
path_clang=clang_64
# The real version for finding macdeployqt
qtversion=5.15.2

deploy=~/Qt/${qtversion}/${path_clang}/bin/macdeployqt

qt_ver_sion=`echo $qtpathversion | sed -e 's/\./_/g'`

toprecoll=$top/recoll/src
appdir=$toprecoll/build-recoll-win-Desktop_Qt_${qt_ver_sion}_${path_clang}bit-Release/recoll.app
rclindexdir=$toprecoll/windows/build-recollindex-Desktop_Qt_${qt_ver_sion}_${path_clang}bit-Release
rclqdir=$toprecoll/windows/build-recollq-Desktop_Qt_${qt_ver_sion}_${path_clang}bit-Release
bindir=$appdir/Contents/MacOS
datadir=$appdir/Contents/Resources

dmg=$appdir/../recoll.dmg

version=`cat $toprecoll/RECOLL-VERSION.txt`

test -d $appdir || fatal Must first have built recoll in $appdir

cp $rclindexdir/recollindex $bindir || exit 1
cp $rclqdir/recollq $bindir || exit 1

cp $top/antiword/antiword $bindir || exit 1
mkdir -p $datadir/antiword || exit 1
cp $top/antiword/Resources/* $datadir/antiword || exit 1


rm -f $dmg ~/Documents/recoll-$version-*.dmg

$deploy $appdir -dmg || exit 1


hash=`(cd $top/recoll;git log -n 1  | head -1  | awk '{print $2}' |cut -b 1-8)`
dte=`date +%Y%m%d`
mv $dmg ~/Documents/recoll-$version-$dte-$hash.dmg || exit 1
ls -l ~/Documents/recoll-$version-*.dmg

