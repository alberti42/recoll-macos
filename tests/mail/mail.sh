#!/bin/sh

topdir=`dirname $0`/..
. $topdir/shared.sh

initvariables $0

(
  recollq -S dbytes -D Outmail_uniqueTerm          
  recollq '"St Pierre en Chartreuse"'
  recollq HtmlAttachment_uniqueTerm
  recollq '"Dear Corporate Administrator"'
  recollq TestTbirdWithoutEmptyLine
  recollq TestTbirdWithEmptyLine
  recollq Utf8attachaccentueaccentue

) 2> $mystderr | egrep -v '^Recoll query: ' > $mystdout

checkresult
