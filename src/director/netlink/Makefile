###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a tcmi libraries                            #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/05/2004                                             #
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

DIRECTOR_COMM_LIB     = director-comm
DIRECTOR_COMM_LIBLOBJ = $(DIRECTOR_COMM_LIB).lo
DIRECTOR_COMM_LIBOBJ  = $(DIRECTOR_COMM_LIB).o
# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=director-comm
 include $(src)/../../debug.make

 # targets that are being built
 obj-m     := $(DIRECTOR_COMM_LIBOBJ)
 obj-m     += director-comm.o
 director-comm-objs := genl_ext.o npm_msg.o node_connected_msg.o task_exitted_msg.o immigration_request_msg.o generic_user_message_recv_msg.o generic_user_message_send_handler.o comm.o


# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(DIRECTOR_COMM_LIBLOBJ): default
	cp $(DIRECTOR_COMM_LIBOBJ) $(DIRECTOR_COMM_LIBLOBJ)


include $(TOPDIR)/src/Rules.make


endif
