###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI comm                                 #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/06/2005                                             #
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

TCMI_COMM     = tcmicomm
TCMI_COMMLOBJ = $(TCMI_COMM).lo
TCMI_COMMOBJ  = $(TCMI_COMM).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-comm-lib
 include $(src)/../../debug.make

# targets that are being built everything defaults to the
# communication library including the entire communication protocol
 obj-m     := $(TCMI_COMMOBJ)
 $(TCMI_COMM)-objs := tcmi_comm.o tcmi_msg.o tcmi_procmsg.o tcmi_transaction.o tcmi_msg_factory.o \
			tcmi_skel_msg.o tcmi_skelresp_msg.o \
			tcmi_skel_procmsg.o tcmi_skelresp_procmsg.o \
			tcmi_err_msg.o tcmi_err_procmsg.o tcmi_vfork_done_procmsg.o \
			tcmi_exit_procmsg.o tcmi_p_emigrate_msg.o tcmi_guest_started_procmsg.o \
			tcmi_ppm_p_migr_back_guestreq_procmsg.o tcmi_ppm_p_migr_back_shadowreq_procmsg.o\
			tcmi_rpc_procmsg.o tcmi_rpcresp_procmsg.o\
			tcmi_authenticate_msg.o tcmi_authenticate_resp_msg.o tcmi_signal_msg.o tcmi_generic_user_msg.o


# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_COMMLOBJ): default
	cp $(TCMI_COMMOBJ) $(TCMI_COMMLOBJ)
include $(TOPDIR)/src/Rules.make

endif
