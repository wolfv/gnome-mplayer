#!/bin/sh

echo -n "Creating the build system... "

libtoolize --copy --force || exit
aclocal || exit
autoheader || exit
automake --copy --force-missing --add-missing --gnu || exit
autoconf || exit

echo "done."
