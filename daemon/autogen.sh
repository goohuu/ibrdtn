#!/bin/bash
#

# create version file
. mkversion.sh $@

# run libtool
libtoolize -i

# run autotools
aclocal
automake --add-missing -c
autoreconf -i
