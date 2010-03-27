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
TOPDIR	:= ../../..

TCMI_MIG     = tcmimig
TCMI_MIGLOBJ = $(TCMI_MIG).lo
TCMI_MIGOBJ  = $(TCMI_MIG).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-migcom
 include $(src)/../../debug.make

 obj-m     := $(TCMI_MIGOBJ)
 $(TCMI_MIG)-objs := tcmi_migcom.o tcmi_mighooks.o tcmi_npm_params.o

# This branch is taken during the first invocation of make
else

SUBDIRS := fs/

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_MIGOBJ): default
	cp $(TCMI_MIGOBJ) $(TCMI_MIGLOBJ)
include $(TOPDIR)/src/Rules.make

endif
