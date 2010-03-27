###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds ProxyFS                                     #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/13/2005                                             #
#                                                                 #
# Last modified: 08/12/2007                                       #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

TOPDIR := ../..

sort_compile: proxyfs_ioctl_table_sorted.o default 

proxyfs_ioctl_table_sorted.s: proxyfs_ioctl_table.s
	$(TOPDIR)/scripts/proxyfs_ioctl_sort.pl < proxyfs_ioctl_table.s > proxyfs_ioctl_table_sorted.s

proxyfs_ioctl_table.s: proxyfs_ioctl_table.c
	gcc -S proxyfs_ioctl_table.c

proxyfs_ioctl_table_sorted.o: proxyfs_ioctl_table_sorted.s
	gcc -c proxyfs_ioctl_table_sorted.s	

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=ProxyFS
 # debugging rules
 include  $(src)/../debug.make

 # test case modules
 obj-m       := proxyfs.o
 proxyfs-objs += proxyfs_module.o proxyfs_fs.o proxyfs_msg.o proxyfs_peer.o buffer.o proxyfs_ioctl_table_sorted.o \
		 proxyfs_real_file.o proxyfs_tty_real_file.o proxyfs_generic_real_file.o proxyfs_pipe_real_file.o \
		 proxyfs_proxy_file.o proxyfs_pipe_proxy_file.o proxyfs_generic_proxy_file.o \
		 proxyfs_file.o proxyfs_task.o proxyfs_client.o proxyfs_server.o 

# This branch is taken during the first invocation of make
else
 # These directories will be built prior to the actual target module 
 DEPDIRS :=	
 # insert all the common rules

 include $(TOPDIR)/src/Rules.make
endif
