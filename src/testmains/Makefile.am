CXXFLAGS ?= @CXXFLAGS@
LIBXAPIAN=@LIBXAPIAN@
XAPIANCXXFLAGS=@XAPIANCXXFLAGS@
XSLT_CFLAGS=@XSLT_CFLAGS@
XSLT_LINKADD=@XSLT_LINKADD@
LIBICONV=@LIBICONV@
INCICONV=@INCICONV@
LIBFAM = @LIBFAM@
RCLLIBVERSION=@RCLLIBVERSION@
X_CFLAGS=@X_CFLAGS@
X_PRE_LIBS=@X_PRE_LIBS@
X_LIBS=@X_LIBS@
X_EXTRA_LIBS=@X_EXTRA_LIBS@
X_LIBX11=@X_LIBX11@
DEFS=@DEFS@

COMMONCPPFLAGS = -I. \
    -I$(top_srcdir)/aspell \
    -I$(top_srcdir)/bincimapmime \
    -I$(top_srcdir)/common \
    -I$(top_srcdir)/index \
    -I$(top_srcdir)/internfile \
    -I$(top_srcdir)/rcldb \
    -I$(top_srcdir)/unac \
    -I$(top_srcdir)/utils \
    -I$(top_srcdir)/xaposix \
    -DBUILDING_RECOLL

AM_CPPFLAGS = -Wall -Wno-unused -std=c++11 \
    $(COMMONCPPFLAGS) \
    $(INCICONV) \
    $(XAPIANCXXFLAGS) \
    $(XSLT_CFLAGS) \
    $(X_CFLAGS) \
    -DRECOLL_DATADIR=\"${pkgdatadir}\" \
    -DREADFILE_ENABLE_ZLIB -DREADFILE_ENABLE_MINIZ -DREADFILE_ENABLE_MD5 \
    -D_GNU_SOURCE \
    $(DEFS)

noinst_PROGRAMS = textsplit

textsplit_SOURCES = trtextsplit.cpp
textsplit_LDADD = ../librecoll.la
