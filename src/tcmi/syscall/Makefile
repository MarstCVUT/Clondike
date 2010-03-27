###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI Syscall forwarding framework         #
#                                                                 #
# Author: Petr Malat                                              #
# Created: 08/20/2007                                             #
#                                                                 #
# Last modified: $Id: Makefile,v 1.4 2007/10/20 14:24:20 stavam2 Exp $                                           #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

# this points to the top directory during the first invocation
TOPDIR	:= ../../..

TCMI_SYSCALL     = tcmisyscall
TCMI_SYSCALLLOBJ = $(TCMI_SYSCALL).lo
TCMI_SYSCALLOBJ  = $(TCMI_SYSCALL).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../..   \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-syscall
 include $(src)/../../debug.make

 obj-m     := $(TCMI_SYSCALLOBJ)
 $(TCMI_SYSCALL)-y := tcmi_rpc.o \

 # Cluster core node support (CCN)
 $(TCMI_SYSCALL)-$(CONFIG_TCMI_CCN) += tcmi_shadow_rpc.o tcmi_shadow_pidman_rpc.o    tcmi_shadow_signal_rpc.o \
							 tcmi_shadow_userident_rpc.o tcmi_shadow_groupident_rpc.o \
							 tcmi_shadow_wait_rpc.o tcmi_shadow_fork_rpc.o
 # Process execution node support (PEN)
 $(TCMI_SYSCALL)-$(CONFIG_TCMI_PEN) += tcmi_guest_rpc.o tcmi_guest_generic_rpc.o tcmi_guest_userident_rpc.o \
		    	 			        tcmi_guest_signal_rpc.o  tcmi_guest_groupident_rpc.o \
	 			       tcmi_syscallhooks.o tcmi_syscallhooks_pidman.o    tcmi_syscallhooks_signal.o \
	                                               tcmi_syscallhooks_userident.o tcmi_syscallhooks_groupident.o \
					tcmi_syscallhooks_other.o tcmi_guest_wait_rpc.o tcmi_guest_fork_rpc.o

 $(TCMI_SYSCALL)-objs := $($(TCMI_SYSCALL)-y)
 
# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_SYSCALLOBJ): default
	cp $(TCMI_SYSCALLOBJ) $(TCMI_SYSCALLLOBJ)
include $(TOPDIR)/src/Rules.make

endif

