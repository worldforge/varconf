#! /bin/sh

builddir=$(pwd)
srcdir=$(dirname $0)
test -z "$srcdir" && srcdir=.
cd $srcdir

autoreconf --install || exit 1

echo Ready to configure
cd $builddir
exit 0
