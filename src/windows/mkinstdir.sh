#!/bin/sh

# Script to make a prototype recoll install directory from locally compiled
# software. *** Needs cygwin ***

##############
# Local values (to be adjusted)
# Target directory where we copy things. 
DESTDIR=c:/recollinst

# Recoll src/build tree
RECOLL=c:/recoll/src

UNRTF=c:/unrtf
ANTIWORD=c:/recolldeps/antiword

CONFIGURATION=Debug
PLATFORM=x64

LIBR=C:/recoll/src/windows/build-librecoll-Desktop_Qt_5_5_0_MinGW_32bit-Debug/debug/librecoll.dll
GUIBIN=C:/Users/Bill/recoll/src/build-recoll-Desktop_Qt_5_5_0_MinGW_32bit-Debug/debug/recoll.exe
RCLIDX=C:/recoll/src/windows/build-recollindex-Desktop_Qt_5_5_0_MinGW_32bit-Debug/debug/recollindex.exe
RCLQ=C:/recoll/src/windows/build-recollq-Desktop_Qt_5_5_0_MinGW_32bit-Debug/debug/recollq.exe

################
# Script:

FILTERS=$DESTDIR/Share/filters

fatal()
{
    echo $*
    exit 1
}

# checkcopy. 
cc()
{
    test -f $1 || fatal $1 does not exist
    cp $1 $2 || exit 1
}

copyrecoll()
{
#    bindir=$RECOLL/windows/$PLATFORM/$CONFIGURATION/
#    cc  $bindir/recollindex.exe         $DESTDIR
#    cc  $bindir/recollq.exe             $DESTDIR
#    cc  $bindir/pthreadVC2.dll          $DESTDIR
    cp $LIBR $DESTDIR || fatal copy librecoll
    cp $GUIBIN $DESTDIR || fatal copy recoll.exe
    cp $RCLIDX $DESTDIR || fatal copy recollindex.exe
    cp $RCLQ $DESTDIR || fatal copy recollq.exe

    cc $RECOLL/sampleconf/fields        $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/fragbuts.xml  $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimeconf      $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimemap       $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimeview      $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/recoll.conf   $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/recoll.qss    $DESTDIR/Share/examples

    cp $RECOLL/filters/*                $FILTERS || fatal Copy Filters failed
    cp $RECOLL/python/recoll/recoll/rclconfig.py $FILTERS || fatal Copy rclconfig.py failed

    cp $RECOLL/qtgui/mtpics/*  $DESTDIR/Share/images
    
    cp $RECOLL/qtgui/i18n/*.qm $DESTDIR/Share/translations

}

copyantiword()
{
    bindir=$ANTIWORD/Win32-only/$PLATFORM/$CONFIGURATION

    test -d $Filters/Resources || mkdir -p $FILTERS/Resources || exit 1

    cc  $bindir/antiword.exe            $FILTERS
    
    cp  $ANTIWORD/Resources/*           $FILTERS/Resources || exit 1
}

copyunrtf()
{
    bindir=$UNRTF/Windows/$PLATFORM/$CONFIGURATION

    cc  $bindir/unrtf.exe               $FILTERS

    test -d $FILTERS/Share || mkdir -p $FILTERS/Share || exit 1
    cp  $UNRTF/outputs/*.conf           $FILTERS/Share || exit 1
    cc  $UNRTF/outputs/SYMBOL.charmap   $FILTERS/Share
}


for d in doc examples filters images translations; do
    test -d $DESTDIR/Share/$d || mkdir -p $DESTDIR/Share/$d ||  exit 1
done

copyrecoll
copyunrtf
copyantiword
