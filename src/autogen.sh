#!/bin/sh

aclocal
libtoolize --copy
automake --add-missing --force-missing --copy
autoconf
