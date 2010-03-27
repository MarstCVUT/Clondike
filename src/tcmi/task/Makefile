###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI task framework                       #
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

TCMI_TASK     = tcmitask
TCMI_TASKLOBJ = $(TCMI_TASK).lo
TCMI_TASKOBJ  = $(TCMI_TASK).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-task
 include $(src)/../../debug.make

 obj-m     := $(TCMI_TASKOBJ)
 $(TCMI_TASK)-objs := tcmi_task.o tcmi_shadowtask.o tcmi_guesttask.o


# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_TASKLOBJ): default
	cp $(TCMI_TASKOBJ) $(TCMI_TASKLOBJ)
include $(TOPDIR)/src/Rules.make

endif
