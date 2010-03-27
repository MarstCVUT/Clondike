# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

TOPDIR := ../..

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -I$(src)/.. -g \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=ccfs
 # debugging rules
 include  $(src)/../debug.make

 # test case modules
 obj-m                    := ccfs.o
 ccfs-y                   := dentry.o inode.o mmap.o main.o file.o super.o read_write.o prefetcher.o filter.o

 
# This branch is taken during the first invocation of make
else
 # These directories will be built prior to the actual target module 
 SUBDIRS := 
 # insert all the common rules
 include $(TOPDIR)/src/Rules.make
endif
