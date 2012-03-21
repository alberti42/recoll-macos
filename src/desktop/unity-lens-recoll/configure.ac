# When releasing also remember to update the soname as instructed below
AC_INIT(unity-lens-recoll, 0.1.1, https://www.recoll.org)
AM_INIT_AUTOMAKE([subdir-objects])
AM_PATH_PYTHON

AC_PREREQ(2.59)

# Variables to escape in .in files
AS_AC_EXPAND(PREFIX, $prefix)
AC_SUBST(PREFIX)

AS_AC_EXPAND(DATADIR, $datarootdir)
AC_SUBST(DATADIR)

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)
AC_SUBST(LIBEXECDIR)

AC_OUTPUT([
bin/unity-recoll-daemon
data/recoll.lens
data/unity-lens-recoll.menu
data/unity-lens-recoll.service
Makefile
])

AC_MSG_NOTICE([

  unity-lens-recoll
  --------------------

  Prefix           : ${prefix}

])

