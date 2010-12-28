##############################################################################
# @file Makefile - Main Clondike sources makefile
#
# Author: Petr Malat
#
# This file is part of Clondike.
#
# Clondike is free software: you can redistribute it and/or modify it under 
# the terms of the GNU General Public License version 2 as published by 
# the Free Software Foundation.
#
# Clondike is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
# details.
# 
# You should have received a copy of the GNU General Public License along with 
# Clondike. If not, see http://www.gnu.org/licenses/.
##############################################################################

ifeq ($(KERNELRELEASE),)


.PHONY: all prepare clean

#KDIR ?= /home/petr/projects/clondike3/linux-2.6.33.1/ # symlink to the kernel source
KDIR ?= /usr/src/linux-2.6.33.1
ARCH := $(shell uname -m | sed -e s/i.86/i386/     -e s/sun4u/sparc64/ \
		               -e s/arm.*/arm/ 	   -e s/sa110/arm/ \
		               -e s/s390x/s390/    -e s/parisc64/parisc/ \
			       -e s/ppc.*/powerpc/ -e s/mips.*/mips/ )

NPWD := $(shell pwd)

all: prepare	
	$(MAKE) -C $(KDIR) M=$(NPWD)
clean:
	$(MAKE) -C $(KDIR) M=$(NPWD) clean
prepare:
	(cd scripts; gcc dbgenv.c -o dbgenv)
	ln -fsn $(ARCH) arch/current
else

include $(src)/debug.make
export PATH := $(PATH):$(NPWD)/$(src)/scripts:$(src)/scripts

subdir-ccflags-y   := -I$(src) 
obj-$(CONFIG_TCMI) := kkc/ director/ ccfs/ proxyfs/ tcmi/ 

endif

