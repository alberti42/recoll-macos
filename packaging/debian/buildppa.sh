#!/bin/sh

RCLVERSION=1.13.04

debdir=debianrclqt3

rm -rf recoll-${RCLVERSION}/debian
cp -rp $debdir recoll-${RCLVERSION}/debian

for series in dapper hardy intrepid jaunty ;do 
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERSION}/debian/changelog ;
  (cd recoll-${RCLVERSION};debuild -S -sa) 
  dput recoll-ppa recoll_${RCLVERSION}-0~ppa1~${series}1_source.changes
done

debdir=debianrclqt4

rm -rf recoll-${RCLVERSION}/debian
cp -rp $debdir recoll-${RCLVERSION}/debian

for series in karmic ; do
  sed -e s/SERIES/$series/g < ${debdir}/changelog > \
    recoll-${RCLVERSION}/debian/changelog ;
  (cd recoll-${RCLVERSION};debuild -S -sa) 
  dput recoll-ppa recoll_${RCLVERSION}-0~ppa1~${series}1_source.changes
done

