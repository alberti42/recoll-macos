
(this used to work but last time I tried with konqueror (kde3, 11-2007), it failed with "protocol not authorized" or such. Didn't investigate more).

This is a small experiment with a recoll kio_slave

A kio_slave was implemented, supporting the "get" operation. Ie, you type 
"recoll: some query terms" in the Konqueror address window, and you receive
a list of results, each with a link to the document.

This does not appear terribly useful as such, especially because I couldn't
think of a way to get access to email documents (especially those inside a
multi-msg file) from Konqueror. The Recoll preview feature is actually
quite useful in this case.

Implementation notes:
-----------------------

- Not using libtool. Probably should. compilation flags in the Makefile
  were copy-pasted from a kdebase compilation tree on FreeBSD (kio/man).

- You MUST install a kio_recoll.la in lib/kde3 along with kio_recoll.so,
  else kdeinit won't be able to load the lib (probably uses the libltdl
  crap?). The one in this directory was duplicated/adjusted from
  kio_man.la

- The current implementation always tries to retrieve 100 docs (doesn't
  even stop if there are less). It would probably be easy to add state and
  previous/next buttons. As I didn't find this thing to be particularly
  useful, I didn't bothered to.

- Also would need a page header, configuration polish etc... Not done for
  the same reason, this is a proof of concept.

- If you want to try, compile, then install kio_recoll.la kio_recoll.so
  wherever kde keeps its plugins (ie: lib/kde3), and recoll.protocol in the
  services directory (share/services ? look for other .protocol file).

- I saw after doing the build/config mockup that kdevelop can generate a kio_slave project. This would certainly be the next thing to do.

