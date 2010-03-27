###################################################################
#                           Rules.make                            #
#                                                                 #
# Description: Common makefile rules used by components that are  #
# 	       built using the Kbuild system                      #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 03/28/2005                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################


KDIR        := /usr/src/linux-2.6.33.1/   # symlink to the kernel source
#KDIR        := /usr/src/linux-2.6.23/   # symlink to the kernel source
#KDIR        := /lib/modules/2.6.21.6/build   # symlink to the kernel source
PWD         := $(shell pwd)
#EXTRA_CFLAGS += -D__MDBG_ON__=1

# before building the actual modules target, try building extra directories
default: subdirs
	$(MAKE) -C $(KDIR) M=$(PWD)

# this is to build depending directories, usually containing lib.a as
# the Kbuild system doesn't like building the lib.a automatically
subdirs:
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir; \
	done


clean:  subdirs_clean subdirs_delete_clean
	rm -f *~  *.lo Module.symvers
	$(MAKE) -C $(KDIR) M=$(PWD) clean


subdirs_clean:
	@for dir in $(SUBDIRS); do \
	$(MAKE) -C $$dir clean; \
	done


subdirs_delete_clean:
	@for dir in $(DEL_SUBDIRS); do \
	rm -fR $$dir; \
	done


# This implicit rule is needed when using Kbuild to put all object
# files into a pseudo module, the extension then needs to be changed
# to .lo. That allows specifying these lib objects as regular object
# files when building regular modules
%.lo : %.o
	cp $< $@ 
