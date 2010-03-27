###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a clondike checkpoint filesystem            #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 12/10/2004                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y

# this points to the top directory during the first invocation
TOPDIR	:= ../../..

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-ctlfs

 include $(src)/../../debug.make

 # targets that are being built
 obj-m       := tcmictlfs.o
 tcmictlfs-objs := tcmi_ctlfs.o tcmi_ctlfs_entry.o tcmi_ctlfs_dir.o tcmi_ctlfs_file.o \
		tcmi_ctlfs_symlink.o  


# This branch is taken during the first invocation of make
else
	include $(TOPDIR)/src/Rules.make
endif
