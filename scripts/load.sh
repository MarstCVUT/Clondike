#!/bin/sh
###################################################################
#                       load.sh                                   #
#                                                                 #
# Description: Loads all subsystems to bring the cluster node up  #
#                                                                 #
# Synopsis:                                                       #
# load.sh                                                         #
#                                                                 #
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: load.sh,v 1.7 2008/05/02 19:59:20 stavam2 Exp $                                                            #
#                                                                 #
###################################################################

echo -n Loading TCMI..
insmod ../src/kkc/kkc_lib.ko
insmod ../src/proxyfs/proxyfs.ko server=tcp:0.0.0.0:1112
insmod ../src/director/director-mod.ko
#insmod ../src/proxyfs/proxyfs.ko
insmod ../src/tcmi/ctlfs/tcmictlfs.ko
insmod ../src/tcmi/ckpt/tcmickptcom.ko
insmod ../src/tcmi/tcmi.ko
insmod ../src/ccfs/ccfs.ko
mount -t tcmi_ctlfs bla /clondike
echo done.
