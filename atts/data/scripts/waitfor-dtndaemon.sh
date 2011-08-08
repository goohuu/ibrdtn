#!/bin/sh
#

while [ 1 == 1 ]; do
	CHECK1=`ps | grep -v 'grep' | grep -v '/bin/sh' | grep dtn | wc -l`
	sleep 5
	CHECK2=`ps | grep -v 'grep' | grep -v '/bin/sh' | grep dtn | wc -l`
	
	if [ $CHECK1 -ge 1 ] && [ $CHECK2 -ge 1 ]; then
		break
	fi
	
	sleep 5
done

