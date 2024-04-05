#!/bin/sh

VERSION=`cat RECOLL-VERSION.txt`

sed -i -E -e '/^#define[ \t]+PACKAGE_VERSION/c\'\
"#define PACKAGE_VERSION \"$VERSION\"" \
common/autoconfig-win.h common/autoconfig-mac.h

