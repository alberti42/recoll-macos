AC_INIT(Recoll, 1.6)
AC_CONFIG_HEADERS([common/autoconfig.h])
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR(index/recollindex.cpp)

sys=`uname`
if test -f mk/$sys ; then
    (cd mk; test -f sysconf || ln -s $sys sysconf)
else
   cat <<EOF
    No system configuration file found in mk/ for '$sys'. Maybe you
    could try to write one, starting from one of the existing files, they
    are really simple.
EOF
   exit 1
fi

# Use aspell to provide spelling expansions ?
# The default is yes. If we do find an aspell installation, we use it. Else
# we do compile the aspell module using an internal copy of aspell.h
# Only --with-aspell=no will completely disable aspell support
AC_ARG_WITH(aspell, 
    AC_HELP_STRING([--without-aspell],
   [Disable use of aspell spelling package to provide term expansion to other spellings]),
        withAspell=$withval, withAspell=yes)
case $withAspell in
     no);;
     yes)
     AC_PATH_PROG(aspellProg, aspell)
     ;;
     *) # The argument should be the path to the aspell program
     aspellProg=$withAspell
     ;;
esac
echo withAspell = $withAspell

if test X$withAspell != Xno ; then
   AC_DEFINE(RCL_USE_ASPELL, 1, [Compile the aspell interface])
   if test X$aspellProg != X ; then
      aspellBase=`dirname $aspellProg`
      aspellBase=`dirname $aspellBase`
      AC_DEFINE_UNQUOTED(ASPELL_PROG, "$aspellProg",
	[Path to the aspell program])
      if test -f $aspellBase/include/aspell.h ; then	
          AC_DEFINE_UNQUOTED(ASPELL_INCLUDE, "$aspellBase/include/aspell.h",
	    [Path to the aspell api include file])
      else
	AC_MSG_NOTICE([aspell support enabled but aspell package not found. Compiling with internal aspell interface file])
	 AC_DEFINE(ASPELL_INCLUDE, ["aspell-local.h"])
      fi
   else
     # aspell support enabled but no aspell install yet
	AC_MSG_NOTICE([aspell support enabled but aspell package not found. Compiling with internal aspell interface file])
	AC_DEFINE(ASPELL_INCLUDE, ["aspell-local.h"])
   fi
fi

# Real time monitoring with FAM
AC_ARG_WITH(fam, 
    AC_HELP_STRING([--with-fam],
   [Use File Alteration Monitor for almost real time indexing of modified files. Give the fam/gamin library as argument (ie: /usr/lib/libfam.so) if configure does not find the right one.]),
        withFam=$withval, withFam=no)
case $withFam in
     no);;
     yes)
	for dir in /usr/local/lib /usr/lib;do 
	 if test -f $dir/libfam.so ; then famLib=$dir/libfam.so;break;fi
	done
     ;;
     *) # The argument should be the path to the fam library
     famLib=$withFam
     ;;
esac

if test X$withFam != Xno ; then
   AC_DEFINE(RCL_MONITOR, 1, [Real time monitoring option])
   AC_DEFINE(RCL_USE_FAM, 1, [Compile the fam interface])
   if test X$famLib != X ; then
      famLibDir=`dirname $famLib`
      famBase=`dirname $famLibDir`
      famBLib=`basename $famLib .so | sed -e s/lib//`
      if test ! -f $famBase/include/fam.h ; then
	 AC_MSG_ERROR([fam.h not found in $famBase/include. Specify --with-fam=no to disable fam support])
      fi
      LIBFAM="-L$famLibDir -l$famBLib"
      AC_MSG_NOTICE([fam library directive: $LIBFAM])
      AC_DEFINE_UNQUOTED(FAM_INCLUDE, "$famBase/include/fam.h",
	[Path to the fam api include file])
   else
	AC_MSG_ERROR([fam library not found])
   fi
fi

if test -f /usr/include/sys/inotify.h -a X$withFam = Xno ; then
   inot_default=yes
else
   inot_default=no
fi

# Real time monitoring with inotify
AC_ARG_WITH(inotify, 
    AC_HELP_STRING([--with-inotify],
   [Use inotify for almost real time indexing of modified files.]),
        withInotify=$withval, withInotify=$inot_default)

if test X$withInotify != Xno ; then
   AC_MSG_NOTICE([enabled support for inotify monitoring])
   AC_DEFINE(RCL_MONITOR, 1, [Real time monitoring option])
   AC_DEFINE(RCL_USE_INOTIFY, 1, [Compile the inotify interface])
else
   AC_MSG_NOTICE([inotify monitoring disabled])
fi

##### Look for iconv. We first look for libiconv in /usr/local/lib:/usr/lib
##    then in libc (Linux, solaris)
AC_LANG(C)
LIBICONV=""
S_LDFLAGS=$LDFLAGS
dir=/usr/local/lib
LDFLAGS="$S_LDFLAGS -L$dir"
unset ac_cv_lib_iconv_iconv_open
AC_CHECK_LIB(iconv, iconv_open, LIBICONV="-L$dir -liconv";INCICONV=-I/usr/local/include)
if test A"$LIBICONV" = A ; then
  dir=/usr/lib
  LDFLAGS="$S_LDFLAGS -L$dir"
  unset ac_cv_lib_iconv_iconv_open
  AC_CHECK_LIB(iconv, iconv_open, LIBICONV="-L$dir -liconv";INCICONV=-I/usr/include)
  if test A"$LIBICONV" = A; then
    dir=/usr/lib
    LDFLAGS="$S_LDFLAGS -L$dir"
    unset ac_cv_lib_iconv_iconv_open
    AC_CHECK_LIB(c, iconv_open, LIBICONV=NONE;INCICONV=-I/usr/include)
  fi
fi
LDFLAGS=$S_LDFLAGS
if test A"$LIBICONV" = A ; then
   AC_MSG_ERROR([Cannot find iconv_open anywhere. Please install iconv])
   exit 1
fi
if test A"$LIBICONV" = ANONE ; then
   LIBICONV=""
fi
#echo LIBICONV $LIBICONV
#echo INCICONV $INCICONV

#### Look for Xapian
AC_PATH_PROG(XAPIAN_CONFIG, xapian-config, no)
if test "$XAPIAN_CONFIG" = "no" ; then
   AC_MSG_ERROR([Cannot find xapian-config command in $PATH. Is
xapian-core installed ?])
   exit 1
fi
LIBXAPIAN=`$XAPIAN_CONFIG --libs`
# Workaround for problem in xapian-config in some versions: wrongly lists
# libstdc++.la in the lib list
for i in $LIBXAPIAN ; do
    case $i in
    *stdc++*|-lm|-lgcc_s|-lc);;
    *) tmpxaplib="$tmpxaplib $i";;
    esac
done
LIBXAPIAN=$tmpxaplib
XAPIANCXXFLAGS=`$XAPIAN_CONFIG --cxxflags`

#echo XAPIAN_CONFIG: $XAPIAN_CONFIG 
#echo LIBXAPIAN: $LIBXAPIAN
#echo XAPIANCXXFLAGS: $XAPIANCXXFLAGS

#### QT
# The way qt and its tools (qmake especially) are installed is very
# different between systems (and maybe qt versions)
#
# In general we need QTDIR to be set, because it is used inside the
# qmake-generated makefiles. But there are exceptions: ie on debian3.1 (at
# least on the sourceforge compile farm), QTDIR is not needed because qmake
# generates hard paths (and is installed in /usr/bin). We don't want to
# force the user to set QTDIR if it is not needed.
#
# The logic is then to first look for qmake, possibly using QTDIR if it is
# set.
# 
# If QTDIR is not set, we then generate a bogus qt project and check if
# QTDIR is needed in the Makefile, in which case we complain.
#
# QMAKESPEC: on most Linux system, there is a 'default' link inside the
# mkspecs directory, so that QMAKESPEC is not needed.
# If QMAKESPEC is not set and needed, the qmake test at the previous test
# will have failed, and we tell the user to check his environment.
#
# Note about qt4: if --enable-qt4 (enables the qt4gui directory), and the 
# QTDIR variable are inconsistent, too bad.
#
AC_ARG_ENABLE(qt4,
    AC_HELP_STRING([--enable-qt4],
        [Use qt version 4]),
        rcl_cv_qt4=$enableval, rcl_cv_qt4=no)
AC_MSG_NOTICE(rcl_cv_qt4 $rcl_cv_qt4)
if test X$rcl_cv_qt4 = Xyes ; then 
   QTGUI=qt4gui
else
   QTGUI=qtgui
fi

if test X$QTDIR != X ; then
   PATH=$QTDIR/bin:$PATH
   export PATH
fi

AC_PATH_PROG([QMAKE], [qmake], NOTFOUND)
if test X$QMAKE = XNOTFOUND ; then 
   AC_MSG_ERROR([Cannot find the qmake program. Maybe you need to install
it and/or set the QTDIR environment variable?])
fi

cd $QTGUI
# We just want a .pro file: no problem with unsubstituted variables at 
# this point.
test -f uifrom3 && sh uifrom3
test -f recoll.pro && chmod +w recoll.pro
cp recoll.pro.in recoll.pro
${QMAKE} recoll.pro
if test $? != 0 ; then
   AC_MSG_ERROR([Cannot use qmake to generate a Makefile. Maybe you need to
check the QTDIR and QMAKESPEC environment variables?])
fi
# is QTDIR set and do we actually need it ?
if test X$QTDIR = X ; then
   QTDIRNEEDED=`grep INCPATH Makefile | grep = | grep QTDIR`
   if test "X$QTDIRNEEDED" != "X" ; then
       AC_MSG_ERROR([You need to set the QTDIR variable to point to the QT
installation. If there is no default mkspecs, you should also set QMAKESPEC])
   fi
fi
cd ..

##################### End QT detection

### X11: this is needed for the session monitoring code (in recollindex -m)
AC_PATH_XTRA
#echo X_CFLAGS "'$X_CFLAGS'" X_PRE_LIBS "'$X_PRE_LIBS'" X_LIBS "'$X_LIBS'" X_EXTRA_LIBS "'$X_EXTRA_LIBS'"

# We have to expand prefix in here, couldn't find a way to do it inside 
# the qt gui .pro file or Makefile. This just means that you can't change
# prefix at build time. It works at install time because we dont' use the
# qtgui Makefile
m_prefix=$prefix
test "X$m_prefix" = "XNONE" && m_prefix=/usr/local
m_datadir=${m_prefix}/share
QTRECOLL_DATADIR=${m_datadir}/recoll

AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(INCICONV)
AC_SUBST(LIBICONV)
AC_SUBST(LIBXAPIAN)
AC_SUBST(LIBFAM)
AC_SUBST(QMAKE)
AC_SUBST(QTGUI)
AC_SUBST(QTRECOLL_DATADIR)
AC_SUBST(XAPIANCXXFLAGS)

AC_CONFIG_FILES(mk/localdefs)
AC_CONFIG_FILES(recollinstall)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(sampleconf/recoll.conf)
AC_CONFIG_FILES($QTGUI/recoll.pro)

for d in bincimapmime index lib query
do 
    rm -f $d/alldeps.stamp
    cp -f /dev/null $d/alldeps
done

(cd lib;test -f Makefile && chmod +w Makefile;sh mkMake)

AC_OUTPUT
chmod a+x recollinstall
