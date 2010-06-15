#!/bin/sh
#
# This script generate all missing files.
#
#

SUBDIRS="./ ibrcommon ibrdtn daemon tools"

for DIR in $SUBDIRS; do
	cd $DIR
	libtoolize
	aclocal
	automake --add-missing
	cd ..
done

autoreconf -i
