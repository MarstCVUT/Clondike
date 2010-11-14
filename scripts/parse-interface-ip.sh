#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Usage: $0 name-of-interface"	
	exit 1
fi

ifconfig $1  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'

