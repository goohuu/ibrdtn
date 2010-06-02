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
UCI=/sbin/uci

. /usr/sbin/dtnd-build-config.sh $TMPCONF

# read uci configuration
BLOB_PATH=`$UCI -q get ibrdtn.storage.blobs`
BUNDLE_PATH=`$UCI -q get ibrdtn.storage.bundles`
LOG_FILE=`$UCI -q get ibrdtn.main.logfile`
ERR_FILE=`$UCI -q get ibrdtn.main.errfile`

# create blob & bundle path
if [ -n "$BLOB_PATH" ]; then
	/bin/mkdir -p $BLOB_PATH
	
	# clean the blob directory on startup
	/bin/rm -f $BLOB_PATH/file*
fi

if [ -n "$BUNDLE_PATH" ]; then
	/bin/mkdir -p $BUNDLE_PATH
fi

LOGGING=""
if [ -n "$LOG_FILE" ]; then
	LOGGING="$LOGGING > $LOG_FILE"
fi

if [ -n "$ERR_FILE" ]; then
	LOGGING="$LOGGING 2> $ERR_FILE"
fi

# run the daemon
echo "${DTND} -c ${TMPCONF} ${LOGGING} &" | /bin/sh
