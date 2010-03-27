###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds TCMI                                        #
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

TOPDIR := ../..

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -I$(src)/.. -g \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi
 # debugging rules
 include  $(src)/../debug.make

 # test case modules
 obj-m                    := tcmi.o
 tcmi-y                   := tcmi_module.o manager/tcmi_man.o manager/tcmi_migman.o \
		             lib/tcmilib.lo lib/tcmi_sock.o  comm/tcmicomm.lo \
			     migration/tcmimig.lo task/tcmitask.lo \
			     syscall/tcmisyscall.lo migration/fs/tcmifs.lo \
				../arch/current/regs.o # TEMPORARY, REMOVED LATER!
 # Cluster core node support (CCN)
 tcmi-$(CONFIG_TCMI_CCN)  += manager/tcmi_ccnman.o manager/tcmi_ccnmigman.o
 # Process execution node support (PEN)
 tcmi-$(CONFIG_TCMI_PEN)  += manager/tcmi_penman.o manager/tcmi_penmigman.o 


# This branch is taken during the first invocation of make
else
 # These directories will be built prior to the actual target module 
 SUBDIRS := lib/ comm/ manager/ ctlfs/ task/ migration/ ckpt/ syscall/
 # insert all the common rules
 include $(TOPDIR)/src/Rules.make
endif
