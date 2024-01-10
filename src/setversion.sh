#!/bin/sh

VERSION=`cat RECOLL-VERSION.txt`

sed -i -E -e '/^#define[ \t]+PACKAGE_VERSION/c\'\
"#define PACKAGE_VERSION \"$VERSION\"" \
-e '/^#define[ \t]+PACKAGE_STRING/c\'\
"#define PACKAGE_STRING \"Recoll $VERSION\"" \
common/autoconfig-win.h common/autoconfig-mac.h

