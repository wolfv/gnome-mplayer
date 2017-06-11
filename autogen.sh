#!/bin/sh

echo -n "Creating the build system... "

intltoolize --copy --force --automake || exit
libtoolize --copy --force --quiet || exit
aclocal || exit
autoheader || exit
automake --copy --force-missing --add-missing --gnu || exit
autoconf || exit

echo "done."
