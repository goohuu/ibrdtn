#!/bin/sh
#

CONTAINER=`/sbin/uci get ibrdtn.storage.container`
CPATH=`/sbin/uci get ibrdtn.storage.path`

if [ -z "$CONTAINER" ]; then
	echo "Storage container not set in uci."
	echo "use: uci set ibrdtn.storage.container=<container-file>"
	exit 1
fi

if [ -z "$CPATH" ]; then
	echo "Storage container mount path not set in uci."
	echo "use: uci set ibrdtn.storage.path=<mount-path>"
	exit 1
fi

if [ "$1" == "-u" ]; then
	/bin/umount $CPATH
	exit 0
fi

if [ -n "`/bin/mount | grep $CPATH`" ]; then
	echo "Container already mounted"
	exit 1
fi

if [ ! -d "$CPATH" ]; then
	echo "Storage container mount path does not exist."
	exit 1
fi

if [ ! -f "$CONTAINER" ]; then
	echo "Storage container file does not exist."
	exit 1
fi

# try to mount the container
/bin/mount -o loop $CONTAINER $CPATH

if [ $? -gt 0 ]; then
	echo -n "can not mount container file. checking... "
	/usr/sbin/e2fsck -p $CONTAINER

	if [ $? -gt 0 ]; then
		echo " error!"
		exit 1
	fi
	echo "done"
else
	exit 0
fi

# try to mount the container
/bin/mount -o loop $CONTAINER $CPATH

if [ $? -gt 0 ]; then
	echo "repair of container file failed."
	exit 1
fi

echo "repair successful. container ready!"
exit 0

