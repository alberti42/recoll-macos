#!/bin/sh

# Script to make a prototype recoll install directory from locally compiled
# software. *** Needs msys or cygwin ***

################################
# Local values (to be adjusted)

# Target directory where we copy things. 
DESTDIR=c:/recollinst

# Recoll src tree
RCL=c:/recoll/src/

# Note: unrtf not under recolldeps because it's a clone from the
# original mercurial repository 
UNRTF=c:/unrtf

# antiword is under recolldeps: it's not really maintained any more
# and has no public repository
ANTIWORD=c:/recolldeps/antiword

CONFIGURATION=Release
PLATFORM=Win32

LIBXAPIAN=c:/recolldeps/xapian/xapian-core-1.2.21/.libs/libxapian-22.dll

MUTAGEN=C:/recolldeps/mutagen-1.31/

# Where to copy the Qt Dlls from:
QTBIN=C:/Qt/5.5/mingw492_32/bin

# Qt arch
QTA=Desktop_Qt_5_5_0_MinGW_32bit

RCLW=$RCL/windows/

LIBR=$RCLW/build-librecoll-${QTA}-Debug/debug/librecoll.dll
GUIBIN=$RCL/build-recoll-win-${QTA}-Debug/debug/recoll.exe
RCLIDX=$RCLW/build-recollindex-${QTA}-Debug/debug/recollindex.exe
RCLQ=$RCLW/build-recollq-${QTA}-Debug/debug/recollq.exe
RCLS=$RCLW/build-rclstartw-${QTA}-Debug/debug/rclstartw.exe


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
    cp $@ || fatal cp $@ failed
}

# Note: can't build static recoll as there is no static qtwebkit (ref: 5.5.0)
copyqt()
{
    for dll in Qt5Cored.dll Qt5Guid.dll Qt5MultimediaWidgetsd.dll \
        Qt5Multimediad.dll Qt5Networkd.dll Qt5OpenGLd.dll \
        Qt5Positioningd.dll Qt5PrintSupportd.dll Qt5Qmld.dll Qt5Quickd.dll \
        Qt5Sensorsd.dll Qt5Sqld.dll Qt5WebChanneld.dll Qt5WebKitWidgetsd.dll \
        Qt5WebKitd.dll Qt5Widgetsd.dll Qt5Xmld.dll icudt54.dll \
        icuin54.dll icuuc54.dll ; do 
        chkcp $QTBIN/$dll $DESTDIR
    done
}

copyxapian()
{
    chkcp $LIBXAPIAN $DESTDIR
}

copyrecoll()
{
#    bindir=$RCL/windows/$PLATFORM/$CONFIGURATION/
#    chkcp  $bindir/recollindex.exe         $DESTDIR
#    chkcp  $bindir/recollq.exe             $DESTDIR
#    chkcp  $bindir/pthreadVC2.dll          $DESTDIR
    chkcp $LIBR $DESTDIR 
    chkcp $GUIBIN $DESTDIR
    chkcp $RCLIDX $DESTDIR
    chkcp $RCLQ $DESTDIR 
    chkcp $RCLS $DESTDIR 

    chkcp $RCL/sampleconf/fields        $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/fragbuts.xml  $DESTDIR/Share/examples
    chkcp $RCL/windows/mimeconf         $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/mimemap       $DESTDIR/Share/examples
    chkcp $RCL/windows/mimeview         $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/recoll.conf   $DESTDIR/Share/examples
    chkcp $RCL/sampleconf/recoll.qss    $DESTDIR/Share/examples

    chkcp $RCL/python/recoll/recoll/rclconfig.py $FILTERS
    chkcp $RCL/filters/*       $FILTERS 
    chkcp $RCL/qtgui/mtpics/*  $DESTDIR/Share/images
    chkcp $RCL/qtgui/i18n/*.qm $DESTDIR/Share/translations
}

copyantiword()
{
    bindir=$ANTIWORD/Win32-only/$PLATFORM/$CONFIGURATION

    test -d $Filters/Resources || mkdir -p $FILTERS/Resources || exit 1
    chkcp  $bindir/antiword.exe            $FILTERS
    chkcp  $ANTIWORD/Resources/*           $FILTERS/Resources
}

copyunrtf()
{
    bindir=$UNRTF/Windows/$PLATFORM/$CONFIGURATION

    test -d $FILTERS/Share || mkdir -p $FILTERS/Share || exit 1
    chkcp  $bindir/unrtf.exe               $FILTERS
    chkcp  $UNRTF/outputs/*.conf           $FILTERS/Share
    chkcp  $UNRTF/outputs/SYMBOL.charmap   $FILTERS/Share
}

copymutagen()
{
    cp -rp $MUTAGEN/build/lib/mutagen $FILTERS
    # chkcp to check that mutagen is where we think it is
    chkcp $MUTAGEN/build/lib/mutagen/mp3.py $FILTERS/mutagen
}

for d in doc examples filters images translations; do
    test -d $DESTDIR/Share/$d || mkdir -p $DESTDIR/Share/$d || \
        fatal mkdir $d failed
done

copyxapian
copyrecoll
copyunrtf
copyantiword
