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


proc=`ls -d /clondike/*/mig/migproc/* 2> /dev/null`
[ ! -z "$proc" ] && {
    echo "Can't unload, following processes still present on the node:"
    for i in $proc; do
	echo $i;
    done;
    exit 1;
}
echo -n Unloading TCMI..
umount /clondike
umount /mnt/proxy
rmmod tcmi
rmmod tcmickptcom
rmmod tcmictlfs
rmmod director-mod
rmmod proxyfs
rmmod kkc_lib
echo done.
