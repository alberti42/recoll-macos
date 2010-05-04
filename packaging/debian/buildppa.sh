#!/bin/sh

RCLVERS=1.13.04
PPAVERS=1

########## QT3
series3=""
series3="dapper hardy intrepid"

debdir=debianrclqt3

rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in  $series3;do 
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) 
  dput recoll-ppa recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

####### QT4
series4=""
#series4="jaunty karmic lucid"
series4="jaunty lucid"

debdir=debianrclqt4
rm -rf recoll-${RCLVERS}/debian
cp -rp $debdir recoll-${RCLVERS}/debian
for series in $series4 ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERS}/debian/changelog ;
  (cd recoll-${RCLVERS};debuild -S -sa) 
  dput recoll-ppa recoll_${RCLVERS}-0~ppa${PPAVERS}~${series}1_source.changes
done

