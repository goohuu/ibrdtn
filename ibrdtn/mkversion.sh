#!/bin/bash
#

# define default version
MAJOR=0
MINOR=3
MICRO=svn

# create a version.inc file
if [ -n "$1" ] && [ -n "$2" ]; then
	MAJOR=$1
	MINOR=$2
	
	if [ -n "$3" ]; then
		MICRO=$3
	fi
fi

# write version to version.inc file
echo "m4_define([PKG_FULL_VERSION], [$MAJOR.$MINOR.$MICRO])" > version.m4
echo "m4_define([PKG_LIB_VERSION], [1:0:0])" >> version.m4
echo "m4_define([PKG_MAJOR_VERSION], [$MAJOR])" >> version.m4
echo "m4_define([PKG_MINOR_VERSION], [$MINOR])" >> version.m4
echo "m4_define([PKG_MICRO_VERSION], [$MICRO])" >> version.m4
