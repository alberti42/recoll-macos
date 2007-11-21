# @(#$Id: shared.sh,v 1.3 2007-11-21 16:34:16 dockes Exp $  (C) 2006 J.F.Dockes

# shared code and variables for all tests

RECOLL_CONFDIR=/Users/dockes/projets/fulltext/testrecoll/config
export RECOLL_CONFDIR

# Call this with the script's $0 as argument
initvariables() {
  tstdata=/home/dockes/projets/fulltext/testrecoll		
  toptmp=${TMPDIR:-/tmp}/recolltsttmp
  myname=`basename $1 .sh`
  mystderr=$toptmp/${myname}.err
  mystdout=$toptmp/${myname}.out
  mydiffs=$toptmp/${myname}.diffs
}

fatal () {
      echo $*
      exit 1
}

checkresult() {
  if test -s "$mydiffs" ; then
    fatal $myname FAILED
  else
    rm -f $mydiffs
    exit 0
  fi
}


