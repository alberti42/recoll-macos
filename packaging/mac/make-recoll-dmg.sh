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

deploy=~/Qt/5.14.2/clang_64/bin/macdeployqt

top=~/Recoll
toprecoll=$top/recoll/src
appdir=$toprecoll/build-recoll-win-Desktop_Qt_5_14_2_clang_64bit-Release/recoll.app
rclindexdir=$toprecoll/windows/build-recollindex-Desktop_Qt_5_14_2_clang_64bit-Release
bindir=$appdir/Contents/MacOS
datadir=$appdir/Contents/Resources

dmg=$appdir/../recoll.dmg

version=`cat $toprecoll/VERSION`

test -d $appdir || fatal Must first have built recoll in $appdir

cp $rclindexdir/recollindex $bindir || exit 1

cp $top/antiword/antiword $bindir || exit 1
mkdir -p $datadir/antiword || exit 1
cp $top/antiword/Resources/* $datadir/antiword || exit 1


rm -f $dmg ~/Documents/recoll-$version-*.dmg

$deploy $appdir -dmg || exit 1



hash=`(cd recoll;git log -n 1  | head -1  | awk '{print $2}' |cut -b 1-8)`

mv $dmg ~/Documents/recoll-$version-$hash.dmg || exit 1
ls -l ~/Documents/recoll-$version-*.dmg

