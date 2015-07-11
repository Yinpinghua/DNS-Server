#! /bin/bash

if [ $# -ne 1 ] ; then
	echo "usage: ./request.sh [domain name]"
	exit -1
fi

cmd=`dig @127.0.0.1 -p 5354 $1 +short`

if [ "$cmd" == "" ] ; then
	echo "DNS request timed out."
else
	echo $cmd
fi
