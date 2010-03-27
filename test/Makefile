###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Builds tcmi test case modules                      #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 03/28/2005                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################

# Set variable DEBUG to 'y' to enable debug messages. Set variable DEBUGFUNC
# to 'y' to enable debug messages at every begining and every end of funcitons
DEBUG = y
DEBUGFUNC = y

TOPDIR := ..

# kbuild system takes this branch (after being recursively invoked
# from the 'kernelmodule' target, src variable is defined by Kbuild
ifneq ($(KERNELRELEASE),)
 EXTRA_CFLAGS += -g -I$(src)/../src \
	-DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4 -DAPP_NAME=tcmi-testing
 # debugging rules
 include  $(src)/../src/debug.make
#  EXTRA_LDFLAGS =  --start-group $(src)/../src/tcmi/comm/lib.a $(src)/../src/tcmi/lib/lib.a --end-group
 # test case modules
 obj-m       += tcmi_ctlfs_test1.o
 obj-m       += tcmi_ctlfs_test2.o
 obj-m       += tcmi_slotvectest.o
 obj-m       += tcmi_queue_test.o
 obj-m       += tcmi_transactiontest.o
 obj-m       += tcmi_msg_testsndr.o tcmi_msg_testrcvr.o tcmi_comm_testsndr.o
 obj-m       += tcmi_msg_factorytest.o
 obj-m       += kkc_test1.o
 obj-m       += kkc_test2.o
 obj-m       += kkc_test_srv_nonblock.o kkc_recv1M.o kkc_send1M.o
 obj-m       += tcmi_ccnmigmantest.o
 obj-m       += tcmi_penmigmantest.o
 obj-m       += tcmi_hookstest.o
 obj-m       += tcmi_ppm_p_shadowtest.o
 obj-m       += tcmi_ppm_p_guesttest.o
 obj-m       += tcmi_signaltest.o
 obj-m       += tcmi_ckptcomtest.o
 obj-m       += tcmi_ckpt_npm_paramstest.o

tcmi_slotvectest-objs := tcmi_slotvec_test.o ../src/tcmi/lib/tcmilib.lo
tcmi_listeningtest-objs += tcmi_listening_test.o 
tcmi_transactiontest-objs := tcmi_transaction_test.o ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo
tcmi_msg_factorytest-objs += tcmi_msg_factory_test.o  ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo
tcmi_msg_testsndr-objs += tcmi_msg_test_sndr.o ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo
tcmi_msg_testrcvr-objs += tcmi_msg_test_rcvr.o ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo
tcmi_comm_testsndr-objs += tcmi_comm_test_sndr.o ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo
tcmi_ccnmigmantest-objs += tcmi_ccnmigman_test.o  ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo \
			../src/tcmi/lib/tcmi_sock.o ../src/tcmi/manager/tcmi_ccnmigman.o \
			../src/tcmi/manager/tcmi_migman.o
tcmi_penmigmantest-objs += tcmi_penmigman_test.o  ../src/tcmi/comm/tcmicomm.lo ../src/tcmi/lib/tcmilib.lo \
			../src/tcmi/lib/tcmi_sock.o ../src/tcmi/manager/tcmi_penmigman.o \
			../src/tcmi/manager/tcmi_migman.o
tcmi_hookstest-objs += tcmi_hooks_test.o
tcmi_ppm_p_shadowtest-objs += tcmi_ppm_p_shadow_test.o ../src/tcmi/task/tcmitask.lo ../src/tcmi/comm/tcmicomm.lo \
			../src/tcmi/lib/tcmilib.lo  ../src/tcmi/migration/tcmimig.lo 
tcmi_ppm_p_guesttest-objs += tcmi_ppm_p_guest_test.o ../src/tcmi/task/tcmitask.lo ../src/tcmi/comm/tcmicomm.lo \
			../src/tcmi/lib/tcmilib.lo 

tcmi_ckptcomtest-objs += tcmi_ckptcom_test.o 

tcmi_ckpt_npm_paramstest-objs += tcmi_ckpt_npm_params_test.o ../src/tcmi/migration/tcmi_npm_params.o

# This branch is taken during the first invocation of make
else
 SUBDIRS = $(TOPDIR)/src/tcmi/comm $(TOPDIR)/src/tcmi/lib $(TOPDIR)/src/tcmi/task $(TOPDIR)/src/tcmi/ckpt
 # insert all the common rules
 include $(TOPDIR)/src/Rules.make

endif