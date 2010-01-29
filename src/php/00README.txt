Recoll PHP extension:

  Author:  Wenqiang Song (wsong.cn, google mail)

This has minimum features for now. 

The sample/ subdirectory has a minimal script which demonstrates the
interface.

Building the extension needs the librcl.a library (recoll-xxx/lib/librcl.a)
to have been built with PIC objects. This will be handled by the Recoll
build system in the future, but, for now, if using gcc, just add -fPIC
-DPIC to LOCALCXXFLAGS inside mk/localdefs, and run "make clean;make" inside
lib/ . For other compilers, adjust to taste :)

The recoll/ subdirectory has the C++ code and the build script
(make.sh). You'll need to "make install" after building.

If you want to clean up the recoll/ directory, you can run phpize --clean
in there.
