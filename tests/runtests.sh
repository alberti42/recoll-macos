#!/bin/sh

if test ! -f shared.sh ; then
    echo must be run in the top test directory
    exit 1
fi

. shared.sh

if test ! x$reroot = x ; then
    rerootResults
fi

iscmd()
{
    cmd=$1
    case $cmd in
    */*)
	if test -x $cmd -a ! -d $cmd ; then return 0; else return 1; fi ;;
    *)
      oldifs=$IFS; IFS=":"; set -- $PATH; IFS=$oldifs
      for d in $*;do test -x $d/$cmd -a ! -d $d/$cmd && \
          iscmdresult=$d/$cmd && return 0;done
      return 1 ;;
    esac
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

checkcmds recollq recollindex pxattr xadump || exit 1

makeindex() {
  echo "Zeroing Index" 
  rm -rf $RECOLL_CONFDIR/xapiandb $RECOLL_CONFDIR/aspdict.*.rws
  echo "Indexing" 
  recollindex -c $RECOLL_CONFDIR -z
}

if test x$noindex = x ; then
  makeindex
fi

# Yes, we could/should use the $toptmp from shared.sh here, but what if
# this is unset ?
toptmp=${TMPDIR:-/tmp}/recolltsttmp
if test -d "$toptmp" ; then
   rm -rf $toptmp/*
else
    mkdir $toptmp || fatal cant create temp dir $toptmp
fi

dirs=`ls -F | grep / | grep -v CVS | grep -v non-auto | grep -v config`

echo
echo "Running query tests:"

for dir in $dirs ; do
    cd $dir && $ECHON "$dir "
    sh `basename $dir`.sh
    cd ..
done

echo
