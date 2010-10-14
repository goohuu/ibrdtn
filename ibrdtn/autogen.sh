#!/bin/bash
#

# create version file
. mkversion.sh $@

# run libtool
libtoolize

# run autotools
aclocal
automake --add-missing
autoreconf
