#!/bin/sh

fatal()
{
    echo $*
    exit 1
}
Usage()
{
    fatal mkinstdir.sh targetdir
}

test $# -eq 1 || Usage

DESTDIR=$1

test -d $DESTDIR || mkdir $DESTDIR || fatal cant create $DESTDIR

# Script to make a prototype recoll install directory from locally compiled
# software. *** Needs msys or cygwin ***

################################
# Local values (to be adjusted)

BUILD=MSVC
#BUILD=MINGW

if test $BUILD = MSVC ; then
    # Recoll src tree
    RCL=c:/users/bill/documents/recoll/src/
    # Recoll dependancies
    RCLDEPS=c:/users/bill/documents/recolldeps-vc/
    QTA=Desktop_Qt_5_14_1_MSVC2017_32bit-Release/release
    LIBXAPIAN=${RCL}windows/build-libxapian-${QTA}/libxapian.dll
    LIBXML=${RCLDEPS}libxml2/libxml2-2.9.4+dfsg1/win32/bin.msvc/libxml2.dll
    LIBXSLT=${RCLDEPS}libxslt/libxslt-1.1.29/win32/bin.msvc/libxslt.dll
    LIBICONV=${RCLDEPS}build-libiconv-Desktop_Qt_5_14_1_MSVC2017_32bit-Release/release/libiconv.dll
    ZLIB=${RCLDEPS}zlib-1.2.11
    QTBIN=C:/Qt/5.14.1/msvc2017/bin
else
    # Recoll src tree
    RCL=c:/recoll/src/
    # Recoll dependancies
    RCLDEPS=c:/recolldeps/
    LIBXAPIAN=${RCLDEPS}xapian-core-1.4.11/.libs/libxapian-30.dll
    ZLIB=${RCLDEPS}zlib-1.2.8
    QTBIN=C:/Qt/Qt5.8.0/5.8/mingw53_32/bin
    QTGCCBIN=C:/qt/Qt5.8.0/Tools/mingw530_32/bin/
    QTA=Desktop_Qt_5_8_0_MinGW_32bit/release
    PATH=$MINGWBIN:$QTGCCBIN:$PATH
    export PATH
fi

# Where to find libgcc_s_dw2-1.dll et all for progs compiled with
# c:/MinGW (as opposed to the mingw bundled with qt). This is the same
# for either a msvc or mingw build of recoll itself.
MINGWBIN=C:/MinGW/bin

RCLW=$RCL/windows/
LIBR=$RCLW/build-librecoll-${QTA}/${qtsdir}/librecoll.dll
GUIBIN=$RCL/build-recoll-win-${QTA}/${qtsdir}/recoll.exe
RCLIDX=$RCLW/build-recollindex-${QTA}/${qtsdir}/recollindex.exe
RCLQ=$RCLW/build-recollq-${QTA}/${qtsdir}/recollq.exe
RCLS=$RCLW/build-rclstartw-${QTA}/${qtsdir}/rclstartw.exe
    
PYTHON=${RCLDEPS}py-python3
UNRTF=${RCLDEPS}unrtf
ANTIWORD=${RCLDEPS}antiword
PYXSLT=${RCLDEPS}pyxslt
PYEXIV2=${RCLDEPS}pyexiv2
MUTAGEN=${RCLDEPS}mutagen-1.32/
EPUB=${RCLDEPS}epub-0.5.2
FUTURE=${RCLDEPS}python2-future
POPPLER=${RCLDEPS}poppler-0.68.0/
LIBWPD=${RCLDEPS}libwpd/libwpd-0.10.0/
LIBREVENGE=${RCLDEPS}libwpd/librevenge-0.0.1.jfd/
CHM=${RCLDEPS}pychm
MISC=${RCLDEPS}misc
LIBPFF=${RCLDEPS}pffinstall
ASPELL=${RCLDEPS}/aspell-0.60.7/aspell-installed

################
# Script:
FILTERS=$DESTDIR/Share/filters

fatal()
{
    echo $*
    exit 1
}

# checkcopy. 
chkcp()
{
    echo "Copying $@"
    cp $@ || fatal cp $@ failed
}

# Note: can't build static recoll as there is no static qtwebkit (ref:
# 5.5.0)
copyqt()
{
    cd $DESTDIR
    PATH=$QTBIN:$PATH
    export PATH
    $QTBIN/windeployqt recoll.exe
    if test $BUILD = MINGW;then
        # Apparently because the webkit part was grafted "by hand" on
	# the Qt set, we need to copy some dll explicitly
	addlibs="Qt5Core.dll Qt5Multimedia.dll \
	   Qt5MultimediaWidgets.dll Qt5Network.dll Qt5OpenGL.dll \
	   Qt5Positioning.dll Qt5PrintSupport.dll Qt5Sensors.dll \
	   Qt5Sql.dll icudt57.dll \
	   icuin57.dll icuuc57.dll libQt5WebKit.dll \
	   libQt5WebKitWidgets.dll \
	   libxml2-2.dll libxslt-1.dll"
        for i in $addlibs;do
            chkcp $QTBIN/$i $DESTDIR
        done
        chkcp $QTBIN/libwinpthread-1.dll $DESTDIR
        chkcp $QTBIN/libstdc++-6.dll $DESTDIR
    fi
}

copypython()
{
    mkdir -p $DESTDIR/Share/filters/python
    cp -rp $PYTHON/* $DESTDIR/Share/filters/python
    chkcp $PYTHON/python.exe $DESTDIR/Share/filters/python/python.exe
}
copyrecoll()
{
    
    chkcp $GUIBIN $DESTDIR
    chkcp $RCLIDX $DESTDIR
    chkcp $RCLQ $DESTDIR 
    chkcp $RCLS $DESTDIR 
    chkcp $ZLIB/zlib1.dll $DESTDIR
    chkcp $LIBXAPIAN $DESTDIR
    if test $BUILD = MINGW;then
	chkcp $LIBR $DESTDIR 
        chkcp $MINGWBIN/libgcc_s_dw2-1.dll $DESTDIR
    else
	chkcp $LIBXML $DESTDIR
	chkcp $LIBXSLT $DESTDIR
#	chkcp $LIBICONV $DESTDIR
    fi
    chkcp $RCL/COPYING                  $DESTDIR/COPYING.txt
    chkcp $RCL/doc/user/usermanual.html $DESTDIR/Share/doc
    chkcp $RCL/doc/user/docbook-xsl.css $DESTDIR/Share/doc
    mkdir -p $DESTDIR/Share/doc/webhelp
    cp -rp $RCL/doc/user/webhelp/docs/* $DESTDIR/Share/doc/webhelp
    chkcp $RCL/sampleconf/fields        $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/fragbuts.xml  $DESTDIR/Share/examples
    chkcp $RCL/windows/mimeconf         $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/mimemap       $DESTDIR/Share/examples
    chkcp $RCL/windows/mimeview         $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/recoll.conf   $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/recoll.qss    $DESTDIR/Share/examples

    chkcp $RCL/python/recoll/recoll/rclconfig.py $FILTERS
    chkcp $RCL/python/recoll/recoll/conftree.py $FILTERS
    rm -f $FILTERS/rclimg*
    chkcp $RCL/filters/*       $FILTERS
    rm -f $FILTERS/rclimg $FILTERS/rclimg.py
#LATER    chkcp $RCLDEPS/rclimg/rclimg.exe $FILTERS
    chkcp $RCL/qtgui/mtpics/*  $DESTDIR/Share/images
    chkcp $RCL/qtgui/i18n/*.qm $DESTDIR/Share/translations
}

copyantiword()
{
    # MS VS
    #bindir=$ANTIWORD/Win32-only/$PLATFORM/$CONFIGURATION
    # MINGW
    bindir=$ANTIWORD/

    test -d $Filters/Resources || mkdir -p $FILTERS/Resources || exit 1
    chkcp  $bindir/antiword.exe            $FILTERS
    chkcp  $ANTIWORD/Resources/*           $FILTERS/Resources
}

copyunrtf()
{
#    bindir=$UNRTF/Windows/$PLATFORM/$CONFIGURATION
     bindir=$UNRTF/Windows/

    test -d $FILTERS/Share || mkdir -p $FILTERS/Share || exit 1
    chkcp  $bindir/unrtf.exe               $FILTERS
    chkcp  $UNRTF/outputs/*.conf           $FILTERS/Share
    chkcp  $UNRTF/outputs/SYMBOL.charmap   $FILTERS/Share
    # libiconv-2 originally comes from mingw
    chkcp $MISC/libiconv-2.dll $FILTERS
}

copymutagen()
{
    cp -rp $MUTAGEN/build/lib/mutagen $FILTERS
    # chkcp to check that mutagen is where we think it is
    chkcp $MUTAGEN/build/lib/mutagen/mp3.py $FILTERS/mutagen
}

copyepub()
{
    cp -rp $EPUB/build/lib/epub $FILTERS
    # chkcp to check that epub is where we think it is
    chkcp $EPUB/build/lib/epub/opf.py $FILTERS/epub
}

# We used to copy the future module to the filters dir, but it is now
# part of the origin Python tree in recolldeps. (2 dirs:
# site-packages/builtins, site-packages/future)
copyfuture()
{
    cp -rp $FUTURE/future $FILTERS/
    cp -rp $FUTURE/builtins $FILTERS/
    # chkcp to check that things are where we think they are
    chkcp $FUTURE/future/builtins/newsuper.pyc $FILTERS/future/builtins
}

copypyexiv2()
{
    cp -rp $PYEXIV2/pyexiv2 $FILTERS
    chkcp $PYEXIV2/libexiv2python.pyd $FILTERS/
}

copypyxslt()
{
    chkcp $PYXSLT/libxslt.py $FILTERS/
    cp -rp $PYXSLT/* $FILTERS
}

copypoppler()
{
    test -d $FILTERS/poppler || mkdir $FILTERS/poppler || \
        fatal cant create poppler dir
    for f in pdftotext.exe pdfinfo.exe pdftoppm.exe \
             freetype6.dll \
             jpeg62.dll \
             libgcc_s_dw2-1.dll \
             libpng16-16.dll \
             libpoppler*.dll \
             libstdc++-6.dll \
             libtiff3.dll \
             zlib1.dll \
             ; do
        chkcp $POPPLER/bin/$f $FILTERS/poppler
    done
}

copywpd()
{
    DEST=$FILTERS/wpd
    test -d $DEST || mkdir $DEST || fatal cant create poppler dir $DEST

    for f in librevenge-0.0.dll librevenge-generators-0.0.dll \
             librevenge-stream-0.0.dll; do
        chkcp $LIBREVENGE/src/lib/.libs/$f $DEST
    done
    chkcp $LIBWPD/src/lib/.libs/libwpd-0.10.dll $DEST
    chkcp $LIBWPD/src/conv/html/.libs/wpd2html.exe $DEST
    chkcp $MINGWBIN/libgcc_s_dw2-1.dll $DEST
    chkcp $MINGWBIN/libstdc++-6.dll $DEST
    chkcp $ZLIB/zlib1.dll $DEST
    chkcp $QTBIN/libwinpthread-1.dll $DEST
}

copychm()
{
    DEST=$FILTERS
    cp -rp $CHM/chm $DEST || fatal "can't copy pychm"
}

copypff()
{
    DEST=$FILTERS
    cp -rp $LIBPFF $DEST || fatal "can't copy pffinstall"
	DEST=$DEST/pffinstall/mingw32/bin
    chkcp $LIBPFF/mingw32/bin/pffexport.exe $DEST
    chkcp $MINGWBIN/libgcc_s_dw2-1.dll $DEST
    chkcp $MINGWBIN/libstdc++-6.dll $DEST
    chkcp $QTBIN/libwinpthread-1.dll $DEST
}

copyaspell()
{
    DEST=$FILTERS
    cp -rp $ASPELL $DEST || fatal "can't copy $ASPELL"
    DEST=$DEST/aspell-installed/mingw32/bin
    # Check that we do have an aspell.exe.
    chkcp $ASPELL/mingw32/bin/aspell.exe $DEST
    chkcp $MINGWBIN/libgcc_s_dw2-1.dll $DEST
    chkcp $MINGWBIN/libstdc++-6.dll $DEST
}

# First check that the config is ok
diff -q $RCL/common/autoconfig.h $RCL/common/autoconfig-win.h || \
    fatal autoconfig.h and autoconfig-win.h differ
VERSION=`cat $RCL/VERSION`
CFVERS=`grep PACKAGE_VERSION $RCL/common/autoconfig.h | \
cut -d ' ' -f 3 | sed -e 's/"//g'`
test "$VERSION" = "$CFVERS" ||
    fatal Versions in VERSION and autoconfig.h differ


echo Packaging version $CFVERS

for d in doc examples filters images translations; do
    test -d $DESTDIR/Share/$d || mkdir -p $DESTDIR/Share/$d || \
        fatal mkdir $d failed
done

#LATER copyaspell
# copyrecoll must stay before copyqt so that windeployqt can do its thing
copyrecoll
copyqt
#LATER copypyxslt
#LATER copypoppler
#LATER copyantiword
#LATER copyunrtf
# Copied into python3 and installed with it
#copyfuture
#LATER copymutagen
# Switched to perl for lack of python3 version
#copypyexiv2
#LATER copywpd
# Chm is now copied into the python tree, which is installed by copypython
#copychm
#LATER copypff

# Note that pychm and pyhwp are pre-copied into the py-python3 python
# distribution directory. The latter also needs olefile and six (also
# copied)
#LATER copypython
