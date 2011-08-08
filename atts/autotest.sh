#!/bin/bash
#
# ATTS - Automatic Testing Tool Suite
#

if [ "`hostname`" == "predator" ]; then
	cd /home/morgenro/openwrt/atts
	/usr/bin/svn up
fi

PYTHON=`/usr/bin/which python`
$PYTHON src/default.py $@

if [ "`hostname`" == "predator" ]; then
	# copy report to htdocs
	/bin/rm -Rf /ibr/trac/projects/optracom/htdocs/testbed/report
	/bin/mkdir -p /ibr/trac/projects/optracom/htdocs/testbed/report
	/bin/cp data/htdocs/* /ibr/trac/projects/optracom/htdocs/testbed/report
fi
