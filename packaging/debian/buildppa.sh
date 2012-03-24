#!/bin/sh
# Packages needed
# sudo apt-get install g++ gnupg dput lintian mini-dinstall yaclc bzr devscripts
# For the kio: (and kdesdk?)
# sudo apt-get install pkg-kde-tools  cdbs

RCLVERS=1.17.0
PPAVERS=0

case $RCLVERS in
    [23]*) PPANAME=recollexp-ppa;;
    1.14*) PPANAME=recoll-ppa;;
    *)     PPANAME=recoll15-ppa;;
esac

####### QT4
debdir=debianrclqt4
series4="lucid maverick natty oneiric precise"

rm -rf recoll-${RCLVERS}/debian
cp -rp ${debdir}/ recoll-${RCLVERS}/debian

for series in $series4 ; do
  if test -f $debdir/control-$series ; then
      cp -f -p $debdir/control-$series recoll-${RCLVERS}/debian/control
  else 
      cp -f -p $debdir/control recoll-${RCLVERS}/debian/control
  fi

  sed -e s/SERIES/${series}/g \
      -e s/PPAVERS/${PPAVERS}/g \
      < ${debdir}/changelog > recoll-${RCLVERS}/debian/changelog

  (cd recoll-${RCLVERS};debuild -S -sa)  || break

  dput $PPANAME recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

### KIO
seriesk="lucid maverick natty oneiric precise"
seriesk=""

debdir=debiankio
rm -rf recoll-${RCLVERS}/debian
cp -rp ${debdir}/ recoll-${RCLVERS}/debian
for series in $seriesk ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) || break
  dput $PPANAME kio-recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done
