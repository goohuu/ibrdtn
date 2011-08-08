#!/bin/bash
#
# ATTS - Automatic Testing Tool Suite
# single test execution
#

PYTHON=`/usr/bin/which python`
$PYTHON src/default.py $@
