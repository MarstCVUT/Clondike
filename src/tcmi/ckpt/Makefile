###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI checkpointing framework              #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/30/2005                                             #
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


# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-ckpt
 include $(src)/../../debug.make

 obj-m     := tcmickptcom.o
 tcmickptcom-objs := tcmi_ckptcom.o tcmi_ckpt.o tcmi_ckpt_openfile.o tcmi_ckpt_vm_area.o\
	../../arch/arch_ids.o ../../arch/current/regs.o ../migration/tcmi_npm_params.o


# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_TASKLOBJ): default
	cp $(TCMI_TASKOBJ) $(TCMI_TASKLOBJ)
include $(TOPDIR)/src/Rules.make

endif
