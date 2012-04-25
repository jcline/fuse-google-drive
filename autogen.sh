#!/bin/sh
LIBTOOLIZE=`which libtoolize || which glibtoolize`

$LIBTOOLIZE
aclocal
automake --add-missing --copy
autoconf
