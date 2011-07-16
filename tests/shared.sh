# @(#$Id: shared.sh,v 1.4 2009-01-06 18:47:33 dockes Exp $  (C) 2006 J.F.Dockes
# shared code and variables for all tests

RECOLL_TESTDATA=/home/dockes/projets/fulltext/testrecoll

RECOLL_CONFDIR=${RECOLL_TESTDATA}/config
export RECOLL_CONFDIR

# Call this with the script's $0 as argument
initvariables() {
  tstdata=${RECOLL_TESTDATA}
  toptmp=${TMPDIR:-/tmp}/recolltsttmp
  myname=`basename $1 .sh`
  mystderr=$toptmp/${myname}.err
  mystdout=$toptmp/${myname}.out
  mydiffs=$toptmp/${myname}.diffs
}

fatal () {
      set -f
      echo
      echo $*
      exit 1
}

checkresult() {
  if test -s "$mydiffs" ; then
    fatal '*** ' $myname FAILED
  else
    rm -f $mydiffs
    # for tests with a local index
    rm -rf xapiandb aspdict.* missing recoll.conf
    exit 0
  fi
}

