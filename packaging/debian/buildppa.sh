#!/bin/sh
# Packages needed
# sudo apt-get install g++ gnupg dput lintian mini-dinstall yaclc bzr devscripts
# For the kio: (and kdesdk?)
# sudo apt-get install pkg-kde-tools  cdbs

# Active series:
# 20.04LTS focal  2025-04
# 22.04LTS jammy  2027-04
# 23.04    lunar  2024-01
# 23.10    mantic 2024-07
SERIES="focal jammy lunar mantic"
SERIES="focal jammy"

PPA_KEYID=7808CE96D38B9201

RCLVERS=1.36.0
GSSPVERS=1.1.1
PPAVERS=3

#
#Y=/y
Y=
RCLSRC=${Y}/home/dockes/projets/fulltext/recoll/src
GSSPSRC=${Y}/home/dockes/projets/fulltext/gssp-recoll
RCLDOWNLOAD=${Y}/home/dockes/projets/lesbonscomptes/recoll

PPANAME=recoll15-ppa
#PPANAME=recollexp-ppa
#PPANAME=recoll-webengine-ppa

echo "PPA: $PPANAME. Type CR if Ok, else ^C"
read rep

fatal()
{
    echo $*; exit 1
}

check_recoll_orig()
{
    if test ! -f recoll_${RCLVERS}.orig.tar.gz ; then 
        cp -p $RCLDOWNLOAD/recoll-${RCLVERS}.tar.gz \
            recoll_${RCLVERS}.orig.tar.gz || \
            fatal "Can find neither recoll_${RCLVERS}.orig.tar.gz nor " \
            "recoll-${RCLVERS}.tar.gz"
    fi
}

####### QT
debdir=debian
series=$SERIES
#series=

if test "X$series" != X ; then
    check_recoll_orig
    test -d recoll-${RCLVERS} || tar xzf recoll_${RCLVERS}.orig.tar.gz
fi

for series in $series ; do

  rm -rf recoll-${RCLVERS}/debian
  cp -rp ${debdir}/ recoll-${RCLVERS}/debian || exit 1

  if test -f $debdir/control-$series ; then
      cp -f -p $debdir/control-$series recoll-${RCLVERS}/debian/control
  else 
      cp -f -p $debdir/control recoll-${RCLVERS}/debian/control
  fi
  if test -f $debdir/rules-$series ; then
      cp -f -p $debdir/rules-$series recoll-${RCLVERS}/debian/rules
  else 
      cp -f -p $debdir/rules recoll-${RCLVERS}/debian/rules
  fi
  if test -f $debdir/patches/series-$series ; then
      cp -f -p $debdir/patches/series-$series recoll-${RCLVERS}/debian/patches/series
  else 
      cp -f -p $debdir/patches/series recoll-${RCLVERS}/debian/patches/series
  fi

  sed -e s/SERIES/${series}/g \
      -e s/PPAVERS/${PPAVERS}/g \
      < ${debdir}/changelog > recoll-${RCLVERS}/debian/changelog

  (cd recoll-${RCLVERS};debuild -d -k$PPA_KEYID -S -sa)  || break

  dput $PPANAME recoll_${RCLVERS}-1~ppa${PPAVERS}~${series}1_source.changes
done



### KIO.
series=$SERIES
series=

debdir=debiankio
topdir=kio-recoll-${RCLVERS}
if test "X$series" != X ; then
    check_recoll_orig
    if test ! -f kio-recoll_${RCLVERS}.orig.tar.gz ; then
        cp -p recoll_${RCLVERS}.orig.tar.gz \
            kio-recoll_${RCLVERS}.orig.tar.gz || exit 1
    fi
    if test ! -d $topdir ; then 
        mkdir temp
        cd temp
        tar xzf ../recoll_${RCLVERS}.orig.tar.gz || exit 1
        mv recoll-${RCLVERS} ../$topdir || exit 1
        cd ..
    fi
fi
for svers in $series ; do
  rm -rf $topdir/debian
  cp -rp ${debdir}/ $topdir/debian || exit 1
  if test -f $debdir/control-$svers ; then
      cp -f -p $debdir/control-$svers $topdir/debian/control
  else 
      cp -f -p $debdir/control $topdir/debian/control
  fi
  sed -e s/SERIES/$svers/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;
  (cd $topdir;debuild -k$PPA_KEYID -S -sa) || exit 1
  dput $PPANAME kio-recoll_${RCLVERS}-1~ppa${PPAVERS}~${svers}1_source.changes
done

### Krunner plugin. Does not build on focal and bionic because of the
### 5.90 min version requested in the CMakeLists.txt. Did not try to
### work around.
series="jammy kinetic lunar"
series=

debdir=debiankrunner
topdir=krunner-recoll-${RCLVERS}
if test "X$series" != X ; then
    check_recoll_orig
    if test ! -f krunner-recoll_${RCLVERS}.orig.tar.gz ; then
        cp -p recoll_${RCLVERS}.orig.tar.gz \
            krunner-recoll_${RCLVERS}.orig.tar.gz || exit 1
    fi
    if test ! -d $topdir ; then 
        mkdir temp
        cd temp
        tar xzf ../recoll_${RCLVERS}.orig.tar.gz || exit 1
        mv recoll-${RCLVERS} ../$topdir || exit 1
        cd ..
    fi
fi
for svers in $series ; do
  rm -rf $topdir/debian
  cp -rp ${debdir}/ $topdir/debian || exit 1
  if test -f $debdir/control-$svers ; then
      cp -f -p $debdir/control-$svers $topdir/debian/control
  else 
      cp -f -p $debdir/control $topdir/debian/control
  fi
  sed -e s/SERIES/$svers/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;
  (cd $topdir;debuild -k$PPA_KEYID -S -sa) || exit 1
  dput $PPANAME krunner-recoll_${RCLVERS}-1~ppa${PPAVERS}~${svers}1_source.changes
done

### GSSP
series=$SERIES
series=

debdir=debiangssp
if test ! -d ${debdir}/ ; then
    rm -f ${debdir}
    ln -s ${GSSPSRC}/debian $debdir
fi
topdir=gssp-recoll-${GSSPVERS}
dload=$RCLDOWNLOAD/downloads
if test "X$series" != X ; then
    if test ! -f gssp-recoll_${GSSPVERS}.orig.tar.gz ; then 
        if test -f gssp-recoll-${GSSPVERS}.tar.gz ; then
          mv gssp-recoll-${GSSPVERS}.tar.gz gssp-recoll_${GSSPVERS}.orig.tar.gz
        else
          if test -f $dload/gssp-recoll-${GSSPVERS}.tar.gz;then
                cp -p $dload/gssp-recoll-${GSSPVERS}.tar.gz \
                   gssp-recoll_${GSSPVERS}.orig.tar.gz || fatal copy
            else
                fatal "Can find neither " \
                      "gssp-recoll_${GSSPVERS}.orig.tar.gz nor " \
                      "$dload/gssp-recoll-${GSSPVERS}.tar.gz"
            fi
        fi
    fi
    test -d $topdir || tar xzf gssp-recoll_${GSSPVERS}.orig.tar.gz || exit 1
fi
for series in $series ; do

   rm -rf $topdir/debian
   cp -rp ${debdir}/ $topdir/debian || exit 1

  sed -e s/SERIES/$series/g \
      -e s/PPAVERS/${PPAVERS}/g \
          < ${debdir}/changelog > $topdir/debian/changelog ;

  (cd $topdir;debuild -k$PPA_KEYID -S -sa) || break

  dput $PPANAME \
      gssp-recoll_${GSSPVERS}-1~ppa${PPAVERS}~${series}1_source.changes

done
