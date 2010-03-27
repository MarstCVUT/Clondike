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

TCMI_LIB     = tcmilib
TCMI_LIBLOBJ = $(TCMI_LIB).lo
TCMI_LIBOBJ  = $(TCMI_LIB).o
# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-lib
 include $(src)/../../debug.make

 # targets that are being built
 obj-m     := $(TCMI_LIBOBJ)
 obj-m     += tcmi_sock.o
 tcmilib-objs := tcmi_slotvec.o


# This branch is taken during the first invocation of make
else

# TCMI library rule - very bad, but the default rule from Rules.make
# gets probably deleted by the Kbuild
$(TCMI_LIBLOBJ): default
	cp $(TCMI_LIBOBJ) $(TCMI_LIBLOBJ)


include $(TOPDIR)/src/Rules.make


endif
