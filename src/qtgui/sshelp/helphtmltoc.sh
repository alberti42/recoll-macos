#!/bin/sh

fn=qhelp.html
while read line; do
      line=`echo $line| sed -e 's/"/\\\\"/g'`
      echo 'tr('"\"$line\""')' +
done  < $fn
