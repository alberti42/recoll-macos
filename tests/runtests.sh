#!/bin/sh

isLinux=0
isWindows=0
sys=`uname`
case $sys in
    Linux) isLinux=1;;
    MINGW*) isWindows=1;;
esac

iswindows()
{
    if test $isWindows -eq 1; then
        return 0
    fi
    return 1
}

       
fatal()
{
    echo $*;exit 1
}

rerootResults()
{
    savedcd=`pwd`
    dirs=`ls -F | grep / | grep -v CVS | grep -v non-auto | grep -v config`
    for dir in $dirs ; do
    	cd $dir
	resfile=`basename $dir`.txt
	sed -i.bak \
	  -e "s!file:///.*/testrecoll/!file://$RECOLL_TESTDATA/!g" \
	  $resfile
    	cd ..
    done

    cd $RECOLL_CONFDIR
    sed -i.bak \
  -e "s!/.*/testrecoll/!$RECOLL_TESTDATA/!g" \
      mimemap

    cd $savedcd
}

testexe()
{
    _tx_cmd=$1
    if test -d "$_tx_cmd"; then
        return 1
    fi
    if test -x "$_tx_cmd"; then
        iscmdresult="$_tx_cmd"
        return 0
    else
        if iswindows; then
            if test -x "$_tx_cmd".exe; then
                iscmdresult="$_tx_cmd".exe
                return 0
            fi
        fi
    fi
    return 1
}

iscmd()
{
    _ic_cmd=$1
    case "$_ic_cmd" in
        */*)
	    if testexe "$_ic_cmd"; then
                return 0
            fi;;
        *)
            oldifs=$IFS; IFS=":"; set -- $PATH; IFS=$oldifs
            for d in "$@";do
                if testexe "$d/$_ic_cmd"; then
                    return 0
                fi
            done
    esac
    return 1
}

checkcmds()
{
    result=0
    for cmd in $*;do
      if iscmd $cmd 
      then 
        echo $cmd is $iscmdresult
      else 
        echo $cmd not found
        result=1
      fi
    done
    return $result
}

makeindex() {
  echo "Zeroing Index" 
  rm -rf $RECOLL_CONFDIR/xapiandb $RECOLL_CONFDIR/aspdict.*.rws
  rm -rf $RECOLL_CONFDIR/ocrcache
  echo "Indexing" 
  recollindex -c $RECOLL_CONFDIR -z
}

if test ! -f shared.sh ; then
    fatal must be run in the top test directory
fi
if iswindows; then
    checkcmds recollq recollindex || exit 1
else
    checkcmds recollq recollindex pxattr xadump pdftk || exit 1
    iscmd pdftk
    pdftk=$iscmdresult
    tmpdir=${RECOLL_TMPDIR:-$TMPDIR}
    case "$pdftk" in
        /snap/*)
            if test X$tmpdir = X -o "$tmpdir" = /tmp;then
                fatal pdftk as snap need '$TMPDIR' to belong to you
            fi
            ;;
    esac
    if test ! x$reroot = x ; then
        rerootResults
    fi
    # Because of the snap thing
    export TMPDIR=$HOME/tmp
fi


# Temp directory for test results
# Make sure this is computed in the same way as in shared.sh
toptmp=${TMPDIR:-/tmp}/recolltsttmp

test X"$toptmp" = X && fatal "empty toptmp??"
test X"$toptmp" = X/ && fatal "toptmp == / ??"
if test -d "$toptmp" ; then
   rm -rf $toptmp/*
fi
mkdir -p $toptmp || fatal cant create temp dir $toptmp

# Unset DISPLAY because xdg-mime may be affected by the desktop
# environment on the X server
unset DISPLAY
export LC_ALL=en_US.UTF-8

RECOLL_TESTS=`pwd`
RECOLL_TESTDATA=${RECOLL_TESTDATA:-/home/dockes/projets/fulltext/testrecoll}
RECOLL_TESTDATA=`echo $RECOLL_TESTDATA | sed -e 's!/$!!'`
export RECOLL_CONFDIR=$RECOLL_TESTS/config/
# Some tests need to access RECOLL_TESTCACHEDIR
export RECOLL_TESTCACHEDIR=$toptmp

sed -e "s,@RECOLL_TESTS@,$RECOLL_TESTS,g" \
    -e "s,@RECOLL_TESTDATA@,$RECOLL_TESTDATA,g" \
    -e "s,@TMPDIR@,$TMPDIR,g" \
    -e "s,@RECOLL_TESTCACHEDIR@,$RECOLL_TESTCACHEDIR,g" \
    < $RECOLL_CONFDIR/recoll.conf.in \
    > $RECOLL_CONFDIR/recoll.conf || exit 1
sed -e "s,@RECOLL_TESTS@,$RECOLL_TESTS,g" \
    -e "s,@RECOLL_TESTDATA@,$RECOLL_TESTDATA,g" \
    -e "s,@RECOLL_TESTCACHEDIR@,$RECOLL_TESTCACHEDIR,g" \
    < $RECOLL_CONFDIR/mimemap.in \
    > $RECOLL_CONFDIR/mimemap || exit 1

if test x$noindex = x ; then
  makeindex
fi


dirs=`ls -F | grep / | grep -v CVS | grep -v non-auto | grep -v config`

echo
echo "Running query tests:"

for dir in $dirs ; do
    cd $dir && echo -n "$dir "
    sh `basename $dir`.sh
    cd ..
done

echo
