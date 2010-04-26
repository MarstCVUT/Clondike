#!/bin/sh
###################################################################
#                          unload.sh                              #
#                                                                 #
# Description: Checks for any migrated process present on the node#
#              Unloads all subsystems to shutdown the cluster node#
#              Note, that the unloading the checkpointing         #
#              component might fail as there still might be       #
#              processes running from a checkpoint image          #
# Synopsis:                                                       #
# unload.sh                                                       #
#                                                                 #
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: unload.sh,v 1.4 2007/11/05 19:38:28 stavam2 Exp $ 
#                                                                 #
###################################################################


proc=`ls -d /clondike/ccn/mig/migproc/* 2> /dev/null`
[ ! -z "$proc" ] && {
    echo "WARN: There are some cluster tasks running on core node. Unloading of some modules is not possible while those tasks are running. After they are all terminated, re-run this script.:"
    for i in $proc; do
	echo $i;
    done;
#    exit 1;
}

echo -n Unloading TCMI..
rmmod tcmi

umount /clondike
umount /mnt/proxy

rmmod tcmickptcom
rmmod tcmictlfs
rmmod director-mod
rmmod proxyfs
rmmod kkc_lib
echo done.
