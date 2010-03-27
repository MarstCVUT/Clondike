###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI migration framework                  #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/26/2005                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

# this points to the top directory during the first invocation
TOPDIR	:= ../../../..

TCMI_FS     = tcmifs
TCMI_FSLOBJ = $(TCMI_FS).lo
TCMI_FSOBJ  = $(TCMI_FS).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../../../ \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-fs
 include $(src)/../../../debug.make

 obj-m     := $(TCMI_FSOBJ)
 $(TCMI_FS)-objs := fs_mounter_register.o 9p_fs_mounter.o 9p_fs_global_mounter.o

# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_FSOBJ): default
	cp $(TCMI_FSOBJ) $(TCMI_FSLOBJ)
include $(TOPDIR)/src/Rules.make

endif
