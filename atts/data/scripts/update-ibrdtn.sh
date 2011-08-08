#!/bin/sh
#
/etc/init.d/ibrdtn stop
opkg --force-depends remove ibrdtnd ibrdtn-tools ibrdtn ibrcommon
opkg update
opkg install ibrdtnd ibrdtn-tools
umount /dev/loop0
losetup -d /dev/loop0
/etc/init.d/ibrdtn start
