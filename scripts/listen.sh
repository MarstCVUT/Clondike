#!/bin/sh
###################################################################
#                       listen.sh                                 #
#                                                                 #
# Description: CCN starts a listening on a specified interface    #
#                                                                 #
# Synopsis:                                                       #
# listen.sh [listen string]                                       #
#                                                                 #
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: listen.sh,v 1.8 2008/05/05 19:50:44 stavam2 Exp $                                                            #
#                                                                 #
###################################################################

# connection string, there is a default value
#listenstr=${1:-'tcp:192.168.1.161:54321'}
listenip=${1:-'192.168.1.162'}
listenport=${2:-'54321'}
listenprot=${3:-'tcp'}
listenstr=${listenprot}:${listenip}:${listenport}

listenfile=/clondike/ccn/listen
mounterdir=/clondike/ccn/mounter/

# check if there is connection file present
[ ! -w $listenfile ] && {
    echo Can\'t listen on \'$listenstr\', missing listen file:\'$connfile\'
    exit 1
}
#mount -t proxyfs tcp:192.168.1.161:1112 /mnt/proxy

echo -n Starting listening on ${listenstr}..
#insmod ../src/proxyfs/proxyfs.ko server=$listenprot:$listenip:1112
#9p mount setup params
echo 9p-global > ${mounterdir}fs-mount
#echo 9p > ${mounterdir}fs-mount
echo $listenip > ${mounterdir}fs-mount-device
echo "aname=/,uname=root,port=5577" > ${mounterdir}fs-mount-options
#end of 9p mount params

mount -t proxyfs tcp:${listenip}:1112 /mnt/proxy

echo $listenstr > $listenfile
echo 
echo done.
