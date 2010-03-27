#!/bin/sh
###################################################################
#                       connect.sh                                #
#                                                                 #
# Description: Connects a PEN to a specified CCN                  #
#                                                                 #
# Synopsis:                                                       #
# connect.sh [connection string]                                  #
#                                                                 #
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: connect.sh,v 1.3 2007/09/02 21:49:42 stavam2 Exp $                                                            #
#                                                                 #
###################################################################

# connection string, there is a default value
#connstr=${1:-'tcp:192.168.0.5:54321'}
connip=${1:-'192.168.1.161'}
connport=${2:-'54321'}
connprot=${3:-'tcp'}
connstr=${connprot}:${connip}:${connport}


connfile=/clondike/pen/connect

# check if there is connection file present
[ ! -w $connfile ] && {
    echo Can\'t connect, missing connection file:\'$connfile\'
    exit 1
}

#echo 9p > /clondike/pen/fs-mount
insmod ../src/proxyfs/proxyfs.ko

echo -n Connecting to $connstr..
echo $connstr > $connfile
echo done.
