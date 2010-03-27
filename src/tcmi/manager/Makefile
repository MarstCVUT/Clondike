###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds a TCMI managers                             #
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

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../..
#-Wno-unused-function

 include $(src)/../../debug.make

 # targets that are being built
 obj-m                      := tcmi_man.o 
 obj-$(CONFIG_TCMI_CCN)     += tcmi_ccnman.o 
 obj-$(CONFIG_TCMI_PEN)     += tcmi_penman.o 
 obj-m                      += tcmi_migman.o 
 obj-$(CONFIG_TCMI_CCN)     += tcmi_ccnmigman.o 
 obj-$(CONFIG_TCMI_PEN)     += tcmi_penmigman.o 


# This branch is taken during the first invocation of make
else
	include $(TOPDIR)/src/Rules.make
endif
