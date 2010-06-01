#!/bin/sh
#
# safety wrapper for IBR-DTN daemon
#
# Tasks:
#  * start IBR-DTN daemon
#  * restart the daemon after a crash
#  * if respawning to fast, then slow down with backoff
#  * check for enough space on disk and delete bundles if necessary.
#  * clean the blob directory on startup
#

DTND=/usr/sbin/dtnd
TMPCONF=/tmp/ibrdtn.config

. /usr/sbin/dtnd-build-config.sh $TMPCONF

# read uci configuration
BLOB_PATH=`uci -q get ibrdtn.storage.blobs`
BUNDLE_PATH=`uci -q get ibrdtn.storage.bundles`
LOG_FILE=`uci -q get ibrdtn.main.logfile`
ERR_FILE=`uci -q get ibrdtn.main.errfile`

# create blob & bundle path
/bin/mkdir -p $BLOB_PATH $BUNDLE_PATH

# clean the blob directory on startup
/bin/rm -f $BLOB_PATH/file*

# run the daemon
$DTND -c $TMPCONF > $LOG_FILE 2> $ERR_FILE &