###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a simple mmap testcase for cldk-ckptfs      #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 11/15/2004                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################


TOPDIR := ..
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
				  -e s/arm.*/arm/ -e s/sa110/arm/ \
				  -e s/s390x/s390/ -e s/parisc64/parisc/ \
				  -e s/ppc.*/powerpc/ -e s/mips.*/mips/ )

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)

# This branch is taken during the first invocation of make
else
SUBDIRS     := arch kkc tcmi proxyfs director ccfs

.PHONY: all
all: prepare subdirs
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir; done
# insert all the common rules
include $(TOPDIR)/src/Rules.make

prepare:
	ln -fsn $(ARCH) arch/current
endif

