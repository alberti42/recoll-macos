
This is a small experiment with a recoll kio_slave

A kio_slave was implemented, supporting the "get" operation. Ie, you type 
"recoll: some query terms" in the Konqueror address window, and you receive
a list of results, each with a link to the document.

This does not appear terribly useful as such, especially because I couldn't
think of a way to get access to email documents from Konqueror (especially
those inside a multi-msg folder file, it's better with maildir). The Recoll
preview feature is actually quite useful in this case.

Building and installing:
-----------------------
This began under KDE3 and might still be made to work, but I only built
with KDE4 and cmake recently.

A kind of recipe:
 - You need the KDE4 core devel packages and cmake installed
 - Extract the source and build recoll normally.
 - In the recoll source, go to kde/kioslave/recoll, then execute:

 cmake CMakeLists.txt
 ccmake .
  # Change the install prefix from /usr/local to /usr, then 'c', 'g'
 make
 sudo make install

 - Note: there may be a way to install to a personal directory and avoid
   being root, and somebody who knows more about kde will have to teach me
   this.

- You should then be able to enter "recoll:" in the konqueror url, and
  perform a recoll search (supposing you have an index already built of
  course, the kio_slave doesn't deal with this for now).


Implementation notes:
-----------------------

- There would be two main ways to do this: 
  - a-la kio_beagle, using listDir() to list entries pointing to the
    different operations or objects (help, status, search result
    entries, bookmarks, whatever...). The nice thing is that the
    results really look like file object in a directory (probably,
    didn't try it actually), no need for look and feel, it's provided by
    KDE 

  - Or a la strigi: all interactions are through html pages and get()
    operations.  Looks less like a normal konqueror file-system
    listing, and needs more html coding but all in all probably
    simpler.

 Recoll is currently doing the html thing. As far as I understand, the
 way to trigger a listdir is to have a inode/directory default mime
 type in the protocol file, and return inode/directory when
 appropriate in mimetype() (look at kio_beagle). Some kde daemon needs
 to be restarted when doing this (the protocol file is cached
 somewhere).

Also would need a page header, configuration polish etc... Not done for
the same reason, this is a proof of concept.

KDE3 notes
- Not using libtool. Probably should. compilation flags in the Makefile
  were copy-pasted from a kdebase compilation tree on FreeBSD (kio/man).
- You MUST install a kio_recoll.la in lib/kde3 along with kio_recoll.so,
  else kdeinit won't be able to load the lib (probably uses the libltdl
  thingy?). The one in this directory was duplicated/adjusted from
  kio_man.la. The contents don't seem too critical, just needs to exist.
- If you want to try, compile, then install kio_recoll.la kio_recoll.so
  wherever kde keeps its plugins (ie: lib/kde3), and recoll.protocol in the
  services directory (share/services ? look for other .protocol file).
- I saw after doing the build/config mockup that kdevelop can generate a
  kio_slave project. This might be the next thing to do. otoh would need to
  separate the kio from the main source to avoid having to distribute 2megs
  of kde build config files.
