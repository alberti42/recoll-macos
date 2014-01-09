#!/bin/sh
# Packages needed
# sudo apt-get install g++ gnupg dput lintian mini-dinstall yaclc bzr devscripts
# For the kio: (and kdesdk?)
# sudo apt-get install pkg-kde-tools  cdbs

RCLVERS=1.19.10
LENSVERS=1.19.10.3543
SCOPEVERS=1.19.10.3543
PPAVERS=4

case $RCLVERS in
    [23]*) PPANAME=recollexp-ppa;;
    1.14*) PPANAME=recoll-ppa;;
    *)     PPANAME=recoll15-ppa;;
esac
#PPANAME=recollexp-ppa

echo "PPA: $PPANAME. Type CR if Ok, else ^C"
read rep

####### QT4
debdir=debian
# Note: no new releases for lucid: no webkit. Or use old debianrclqt4 dir.
series="precise quantal raring saucy"
#series="precise saucy"

if test "X$series" != X ; then
    test -d recoll-${RCLVERS} || tar xvzf recoll_${RCLVERS}.orig.tar.gz
fi

for series in $series ; do

  rm -rf recoll-${RCLVERS}/debian
  cp -rp ${debdir}/ recoll-${RCLVERS}/debian || exit 1

  if test -f $debdir/control-$series ; then
      cp -f -p $debdir/control-$series recoll-${RCLVERS}/debian/control
  else 
      cp -f -p $debdir/control recoll-${RCLVERS}/debian/control
  fi

  sed -e s/SERIES/${series}/g \
      -e s/PPAVERS/${PPAVERS}/g \
      < ${debdir}/changelog > recoll-${RCLVERS}/debian/changelog

  (cd recoll-${RCLVERS};debuild -S -sa)  || break

  dput $PPANAME recoll_${RCLVERS}-1~ppa${PPAVERS}~${series}1_source.changes
done

### KIO
series="precise quantal raring saucy"
series=

debdir=debiankio
topdir=kio-recoll-${RCLVERS}
if test "X$series" != X ; then
    if test ! -d kio-recoll_${RCLVERS}.orig.tar.gz ; then
        cp -p recoll_${RCLVERS}.orig.tar.gz \
            kio-recoll_${RCLVERS}.orig.tar.gz || exit 1
    fi
    if test ! -d $topdir ; then 
        mkdir temp
        cd temp
        tar xvzf ../recoll_${RCLVERS}.orig.tar.gz || exit 1
        mv recoll-${RCLVERS} ../$topdir || exit 1
        cd ..
    fi
fi
for series in $series ; do

  rm -rf $topdir/debian
  cp -rp ${debdir}/ $topdir/debian || exit 1

  sed -e s/SERIES/$series/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;

  (cd $topdir;debuild -S -sa) || break

  dput $PPANAME kio-recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes

done

### Unity Lens
series="precise quantal raring"
series=

debdir=debianunitylens
topdir=recoll-lens-${LENSVERS}
if test "X$series" != X ; then
    test -d $topdir ||  tar xvzf recoll-lens_${LENSVERS}.orig.tar.gz
fi

for series in $series ; do

   rm -rf $topdir/debian
   cp -rp ${debdir}/ $topdir/debian || exit 1

  sed -e s/SERIES/$series/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;

  (cd $topdir;debuild -S -sa) || break

  dput $PPANAME \
      recoll-lens_${LENSVERS}-1~ppa${PPAVERS}~${series}1_source.changes

done

### Unity Scope
series="saucy"
series=

debdir=debianunityscope
topdir=unity-scope-recoll-${SCOPEVERS}
if test "X$series" != X ; then
    test -d $topdir ||  tar xvzf unity-scope-recoll_${LENSVERS}.orig.tar.gz
fi
for series in $series ; do

   rm -rf $topdir/debian
   cp -rp ${debdir}/ $topdir/debian || exit 1

  sed -e s/SERIES/$series/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;

  (cd $topdir;debuild -S -sa) || break

  dput $PPANAME \
      unity-scope-recoll_${SCOPEVERS}-1~ppa${PPAVERS}~${series}1_source.changes

done
