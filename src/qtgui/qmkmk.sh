#!/bin/sh
# This is used with meson for the recoll custom_target().
# This just runs qmake then make
# I could find no way to do this in 2 steps inside meson
set -e
pro_file=$1

dir=`dirname $pro_file`
fn=`basename $pro_file`

QMAKE=${QMAKE:-qmake}
MAKE=${MAKE:-make}

ncpus=2
if test -f /proc/cpuinfo; then
    ncpus=`grep -E '^processor[ 	]*:' /proc/cpuinfo | wc -l`
elif which sysctl > /dev/null;then
    ncpus=`sysctl hw.ncpu | awk '{print $3}'`
fi

cd $dir
${QMAKE} $fn
${MAKE} -j $ncpus
