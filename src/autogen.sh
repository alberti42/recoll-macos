#!/bin/sh

set -x

aclocal

if test X"$HOMEBREW_ENV" != X; then
    glt=`which glibtoolize`
fi
if test X"$glt" != X; then
    $glt --copy
else
    libtoolize --copy
fi

automake --add-missing --force-missing --copy
autoconf
# Our ylwrap gets clobbered by the above.
git checkout ylwrap
