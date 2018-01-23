#!/bin/sh

aclocal
libtoolize --copy
automake --add-missing --force-missing --copy
autoconf
# Our ylwrap gets clobbered by the above.
git checkout ylwrap
