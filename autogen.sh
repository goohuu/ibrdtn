#!/bin/sh
#
# This script generate all missing files.
#
#

LOCALDIR=`pwd`
SUBDIRS="./ ibrcommon ibrdtn daemon tools"

for DIR in $SUBDIRS; do
	echo "## enter directory $DIR ##"
	cd $DIR
	if [ -e "mkversion.sh" ]; then
		echo "## run mkversion.sh script ##"
		. mkversion.sh $@
	fi 
	
	echo "## run libtoolize ##"
	libtoolize

	echo "## run aclocal ##"
	aclocal

	echo "## run automake ##"
	automake --add-missing
	cd $LOCALDIR
	echo "## leave directory $DIR ##"
done

autoreconf -i
