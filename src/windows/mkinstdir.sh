#!/bin/sh

# Script to make a prototype recoll install directory from locally compiled
# software. *** Needs cygwin ***

##############
# Local values (to be adjusted)
# Target directory where we copy things. 
DESTDIR=/cygdrive/c/recollinst

# Recoll src/build tree
RECOLL=/cygdrive/c/recoll/src

UNRTF=/cygdrive/c/unrtf
ANTIWORD=/cygdrive/c/recolldeps/antiword

CONFIGURATION=Debug
PLATFORM=x64


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
    bindir=$RECOLL/windows/$PLATFORM/$CONFIGURATION/

    cc  $bindir/recollindex.exe         $DESTDIR
    cc  $bindir/recollq.exe             $DESTDIR
    cc  $bindir/pthreadVC2.dll          $DESTDIR

    cc $RECOLL/sampleconf/fields        $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/fragbuts.xml  $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimeconf      $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimemap       $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/mimeview      $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/recoll.conf   $DESTDIR/Share/examples
    cc $RECOLL/sampleconf/recoll.qss    $DESTDIR/Share/examples

    cp $RECOLL/filters/*                $FILTERS || exit 1
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


test -d $DESTDIR || mkdir -p $DESTDIR || exit 1
test -d $DESTDIR/Share/examples || mkdir -p $DESTDIR/Share/examples ||  exit 1
test -d $FILTERS || mkdir -p $FILTERS || exit 1
copyrecoll
copyunrtf
copyantiword
