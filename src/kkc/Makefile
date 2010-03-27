###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds KKC                                         #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 04/13/2005                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
#DEBUG = y
#DEBUGFUNC = y

TOPDIR := ../..

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -I$(src)/.. \
	 -DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=0 -DAPP_NAME=kkc
 # debugging rules
 include  $(src)/../debug.make

 # test case modules
 obj-m       := kkc_lib.o
 kkc_lib-objs += kkc.o kkc_sock.o kkc_sock_tcp.o

# This branch is taken during the first invocation of make
else
 # These directories will be built prior to the actual target module 
 DEPDIRS :=	
 # insert all the common rules
 include $(TOPDIR)/src/Rules.make
endif
