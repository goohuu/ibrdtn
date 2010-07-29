#!/bin/bash
#

# define default version
MAJOR=0
MINOR=4
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
echo "$MAJOR.$MINOR.$MICRO" > version.inc
