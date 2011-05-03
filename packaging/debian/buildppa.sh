#!/bin/sh
# Packages needed
# sudo apt-get install g++ gnupg dput lintian mini-dinstall yaclc bzr devscripts
# For the kio: (and kdesdk?)
# sudo apt-get install pkg-kde-tools  cdbs

RCLVERS=1.15.8
PPAVERS=1

case $RCLVERS in
    1.14*) PPANAME=recoll-ppa;;
    *)     PPANAME=recoll15-ppa;;
esac

########## QT3
series3=""
case $RCLVERS in
    1.14*)
        series3="dapper hardy";;
esac
        
if test X$series3 != X; then
  debdir=debianrclqt3

  rm -rf recoll-${RCLVERS}/debian
  cp -rp $debdir recoll-${RCLVERS}/debian
  for series in  $series3;do 
    sed -e s/SERIES/$series/g < ${debdir}/changelog > \
        recoll-${RCLVERS}/debian/changelog ;
    (cd recoll-${RCLVERS};debuild -S -sa) || break
    dput $PPANAME recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
  done
fi

####### QT4
series4=""
case $RCLVERS in
    1.14*)
        series4="jaunty karmic lucid maverick natty";;
    *)
        series4="jaunty karmic lucid maverick natty";;
esac

debdir=debianrclqt4
rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in $series4 ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa)  || break
  dput $PPANAME recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

### KIO
seriesk=""
seriesk="jaunty karmic lucid maverick natty"

debdir=debiankio
rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in $seriesk ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) || break
  dput $PPANAME kio-recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done
