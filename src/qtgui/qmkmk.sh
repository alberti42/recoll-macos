#!/bin/sh
# This is used with meson for the recoll custom_target().
# This just runs qmake then make
# I could find no way to do this in 2 steps inside meson
set -x
pro_file=$1

dir=`dirname $pro_file`
fn=`basename $pro_file`

QMAKE=${QMAKE:-qmake}
MAKE=${MAKE:-make}

cd $dir
${QMAKE} $fn
${MAKE}
