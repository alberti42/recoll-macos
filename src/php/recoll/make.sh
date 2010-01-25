#!/bin/sh
phpize --clean
phpize
rm aclocal.m4 -rf
autoreconf
./configure --enable-recoll
make -j3
