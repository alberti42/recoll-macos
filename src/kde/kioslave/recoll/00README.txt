Recoll KIO slave
================

This is a small experiment with a recoll KIO slave

It does not appear terribly useful as such, especially because I
couldn't think of a way to get access to email documents from
Konqueror (especially those inside a multi-msg folder file, it's
better with maildir). The Recoll preview feature is actually quite
useful in this case. But anyway, here it is...

Building and installing:
=======================

This began under KDE3 and might still be made to work, but I only
built with KDE4 and cmake recently. Full functionality is only
available with KDE 4.1 and later.

The main Recoll installation shares its prefix with the KIO slave,
which needs to use the KDE one. This means that, if KDE lives in /usr,
Recoll must be configured with --prefix=/usr, not /usr/local. Else
you'll have run-time problems, the slave will not be able to find the
recoll configuration.

A kind of recipe:
 - You need the KDE4 core devel packages and cmake installed

 - Extract the recoll source, configure recoll with --prefix=/usr
   build and install recoll (or use another prefix if KDE lives elsewhere).

 - In the recoll source, go to kde/kioslave/recoll, then execute:

 mkdir builddir
 cd builddir
 cmake .. -DCMAKE_INSTALL_PREFIX=/usr
 make
 sudo make install

 - You should have a look at where "make install" copies things,
   because wrong targets are frequent. Especially, you should check
   that kio_recoll.so is copied to the right place, meaning among the
   output of "kde4-config --path module". As an additional check,
   there should be many other kio_[xxx].so in there. Same for the
   protocol file, check that it's not alone in its directory (really,
   this sounds strange, but, to this point, I've seen more systems
   with broken cmake/KDE configs than correct ones).

You need to build/update the index with recollindex, the KIO slave
doesn't deal with it for now.


Misc build problems:
===================

KUBUNTU 8.10 (updated to 2008-27-11)
------------------------------------
cmake generates a bad dependancy on
      /build/buildd/kde4libs-4.1.2/obj-i486-linux-gnu/lib/libkdecore.so 
inside CMakeFiles/kio_recoll.dir/build.make 

Found no way to fix this. You need to edit the line and replace the
/build/[...]/lib with /usr/lib. This manifests itself with the
following error message:

   make[2]: *** No rule to make target `/build/buildd/kde4libs-4.1.2/obj-i486-linux-gnu/lib/libkdecore.so', needed by `lib/kio_recoll.so'.  Stop.



Usage
=====
You should then be able to enter "recoll:" in the konqueror URL
entry. Depending on the KDE version, this will bring you either to an
HTML search form, or to a directory listing, where you should read the
help file, which explains how to switch between the HTML and
directory-oriented interfaces.


