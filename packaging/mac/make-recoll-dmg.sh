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

#### Adjustable things
top=~/Recoll
# The possibly bogus version we have in paths (may be harcoded in the .pro)
#qcbuildloc=Desktop_Qt_5_15_2_clang_64bit;deploy=~/Qt/5.15.2/clang_64bit/macdeployqt
qcbuildloc=Qt_6_4_2_for_macOS
deploy=~/Qt/6.4.2/macos/bin/macdeployqt

#### Locations depending on the above, should not change much
toprecoll=$top/recoll/src
appdir=$toprecoll/build-recoll-${qcbuildloc}-Release/recoll.app
rclindexdir=$toprecoll/build-recollindex-${qcbuildloc}-Release
rclqdir=$toprecoll/build-recollq-${qcbuildloc}-Release
bindir=$appdir/Contents/MacOS
datadir=$appdir/Contents/Resources
# Target
dmg=$appdir/../recoll.dmg

### Do it
version=`cat $toprecoll/RECOLL-VERSION.txt`

test -d $appdir || fatal Must first have built recoll in $appdir

cp $rclindexdir/recollindex $bindir || exit 1
cp $rclqdir/recollq $bindir || exit 1

# Antiword
cp $top/antiword/antiword $bindir || exit 1
mkdir -p $datadir/antiword || exit 1
cp $top/antiword/Resources/* $datadir/antiword || exit 1

# Poppler: slight modification and special build of poppler 24.04.0 with static linking of
# pdfinfo and pdftotext, see the 00-JF-README.txt in the relocatable-for-recoll branch in the
# local source.  poppler-data is a copy of ubuntu 22.04 /usr/share/poppler
cp $top/poppler/build/utils/pdfinfo $top/poppler/build/utils/pdftotext $bindir || exit 1
mkdir -p $datadir/poppler || exit 1
cp -rp $top/poppler-data/* $datadir/poppler || exit 1

### Create the dmg
rm -f $dmg ~/Documents/recoll-$version-*.dmg

$deploy $appdir -dmg || exit 1

### Rename the dmg according to date and version.
hash=`(cd $top/recoll;git log -n 1  | head -1  | awk '{print $2}' |cut -b 1-8)`
dte=`date +%Y%m%d`
mv $dmg ~/Documents/recoll-$version-$dte-$hash.dmg || exit 1
ls -l ~/Documents/recoll-$version-*.dmg

