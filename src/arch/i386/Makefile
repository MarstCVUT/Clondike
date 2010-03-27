###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds arch specific files                                         #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

TOPDIR := ../../..

ARCH_SPECIFIC = arch_specific
ARCH_SPECIFIC_OBJ = $(ARCH_SPECIFIC).o

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../.. \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=3 -DAPP_NAME=arch_specific
 # debugging rules
 include  $(src)/../../debug.make

 
 obj-m       := $(ARCH_SPECIFIC_OBJ)
 $(ARCH_SPECIFIC)-objs := regs.o
 
# This branch is taken during the first invocation of make
else
 # These directories will be built prior to the actual target module 
 DEPDIRS :=	
 # insert all the common rules
 include $(TOPDIR)/src/Rules.make

endif