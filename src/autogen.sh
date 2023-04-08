#!/bin/sh

set -x

aclocal

# detect libtoolize on linux or glibtoolize in some systems
if (libtoolize --version) < /dev/null > /dev/null 2>&1; then
  LIBTOOLIZE=libtoolize
elif (glibtoolize --version) < /dev/null > /dev/null 2>&1; then
  LIBTOOLIZE=glibtoolize
else
  echo "libtoolize or glibtoolize was not found! Please install libtool." 1>&2
  exit 1
fi

# Back up ylwrap, which will be clobbered by autotools
if [ -f ylwrap ]; then
    cp -fp ylwrap .ylwrap.bak
fi

$LIBTOOLIZE --copy
automake --add-missing --force-missing --copy
autoconf

if [ -f .ylwrap.bak ]; then
  mv -f .ylwrap.bak ylwrap
fi

