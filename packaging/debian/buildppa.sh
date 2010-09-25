#!/bin/sh
# Packages needed
# sudo apt-get install g++ gnupg dput lintian mini-dinstall yaclc bzr devscripts
# For the kio: (and kdesdk?)
# sudo apt-get install pkg-kde-tools  cdbs

RCLVERS=1.14.2
PPAVERS=1

########## QT3
series3=""
series3="dapper hardy"

debdir=debianrclqt3

rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in  $series3;do 
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) || break
  dput recoll-ppa recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

####### QT4
series4=""
series4="jaunty karmic lucid"

debdir=debianrclqt4
rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in $series4 ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa)  || break
  dput recoll-ppa recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

### KIO
seriesk=""
seriesk="jaunty karmic lucid"

debdir=debiankio
rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in $seriesk ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) || break
  dput recoll-ppa kio-recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done
