Recoll krunner module
====================

The recoll krunner module provides results from an existing recoll index to krunner.


Building and installing:
=======================

The main Recoll installation shares its prefix with the KIO slave, which needs to use the KDE
one. This means that, if KDE lives in /usr, Recoll must be configured with --prefix=/usr, not
/usr/local. Else you'll have run-time problems, the slave will not be able to find the Recoll
configuration.

By default, Recoll now installs its shared library in the normal $libdir place (it used to use a
private library installed in a subdirectory). This can be overriden by the configuration.

By default the krunner build will use the public shared library (and its installed include files).

Recipe:

- Make sure the KF5 (or KF6) core and KRunner devel packages and cmake are installed. You probably
  need the kio-devel and extra-cmake-modules packages.

- Extract the Recoll source.

- IF Recoll (esp. the library and include files) is not installed yet: configure recoll with
  --prefix=/usr (or wherever KDE lives), build and install Recoll.

- In the Recoll source, go to kde/krunner, then build and install the krunner module:

    mkdir builddir
    cd builddir
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make
    sudo make install
