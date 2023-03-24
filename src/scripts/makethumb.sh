#!/bin/sh

# Sample thumbnail-creating script suitable as recoll thumbnailercmd

fatal()
{
    echo $*;exit 1
}
usage()
{
    fatal "Usage: makethumb.sh <URL> <MIME> <SIZE> <OUTPATH>"
}

test $# = 4 || usage
in=$1
mime=$2
size=$3
out=$4

set -x
case "$mime" in
    video/*) ffmpegthumbnailer -i"$in" -s"$size" -o"$out";;
    application/pdf)
        # Note that you will have to disable apparmor for evince for this to work. See for example
        # https://linuxconfig.org/how-to-disable-apparmor-on-ubuntu-20-04-focal-fossa-linux
        evince-thumbnailer -s "$size" "$in" "$out";;
    *) exit 1;;
esac
