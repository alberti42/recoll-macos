AC_INIT(Recoll, 1.0)
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
XAPIANCXXFLAGS=`$XAPIAN_CONFIG --cxxflags`

#echo XAPIAN_CONFIG $XAPIAN_CONFIG 
#echo LIBXAPIAN $LIBXAPIAN
#echo XAPIANCXXFLAGS $XAPIANCXXFLAGS

AC_SUBST(LIBXAPIAN)
AC_SUBST(XAPIANCXXFLAGS)
AC_SUBST(LIBICONV)
AC_SUBST(INCICONV)

AC_CONFIG_FILES(mk/localdefs)
AC_CONFIG_FILES(recollinstall)
AC_CONFIG_FILES(sampleconf/recoll.conf)

for d in bincimapmime index lib query
do 
    rm -f $d/alldeps.stamp
    cp -f /dev/null $d/alldeps
done

AC_OUTPUT
chmod a+x recollinstall
