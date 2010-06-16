#!/bin/sh
#
# This script generate all missing files.
#
#

LOCALDIR=`pwd`
SUBDIRS="./ ibrcommon ibrdtn daemon tools"

for DIR in $SUBDIRS; do
	cd $DIR
	if [ -e "mkversion.sh" ]; then
		. mkversion.sh $@
	fi 
	
	libtoolize
	aclocal
	automake --add-missing
	cd $LOCALDIR
done

autoreconf -i
