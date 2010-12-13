##############################################################################
# @file Makefile - Builds Task Checkpointing and Migration Infrastructure
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

ccflags-y = `dbgenv $<`

obj-$(CONFIG_TCMI) := ctlfs/ ckpt/ tcmi.o

tcmi-objs := tcmi_module.o 
########## Manager component
tcmi-objs               += manager/tcmi_man.o manager/tcmi_migman.o
tcmi-$(CONFIG_TCMI_CCN) += manager/tcmi_ccnman.o manager/tcmi_ccnmigman.o
tcmi-$(CONFIG_TCMI_PEN) += manager/tcmi_penman.o manager/tcmi_penmigman.o 
########## Task component
tcmi-objs += task/tcmi_guesttask.o task/tcmi_shadowtask.o task/tcmi_task.o 
########## Migration component
tcmi-objs += migration/tcmi_migcom.o migration/tcmi_mighooks.o migration/tcmi_npm_params.o 
tcmi-objs += migration/fs/fs_mounter_register.o migration/fs/9p_fs_global_mounter.o migration/fs/9p_fs_mounter.o
########## Syscall component
tcmi-objs += syscall/tcmi_rpc.o 
tcmi-$(CONFIG_TCMI_CCN) += syscall/tcmi_shadow_rpc.o syscall/tcmi_shadow_pidman_rpc.o syscall/tcmi_shadow_signal_rpc.o \
	syscall/tcmi_shadow_userident_rpc.o syscall/tcmi_shadow_groupident_rpc.o syscall/tcmi_shadow_wait_rpc.o \
	syscall/tcmi_shadow_fork_rpc.o
tcmi-$(CONFIG_TCMI_PEN) += syscall/tcmi_guest_rpc.o syscall/tcmi_guest_generic_rpc.o syscall/tcmi_guest_userident_rpc.o \
	syscall/tcmi_guest_signal_rpc.o  syscall/tcmi_guest_groupident_rpc.o syscall/tcmi_syscallhooks.o \
	syscall/tcmi_syscallhooks_pidman.o syscall/tcmi_syscallhooks_signal.o syscall/tcmi_syscallhooks_userident.o \
       	syscall/tcmi_syscallhooks_groupident.o syscall/tcmi_syscallhooks_other.o syscall/tcmi_guest_wait_rpc.o \
	syscall/tcmi_guest_fork_rpc.o
########## Lib component
tcmi-objs += lib/tcmi_sock.o lib/tcmi_slotvec.o
########## Comm component
tcmi-objs += comm/tcmi_comm.o comm/tcmi_msg.o comm/tcmi_procmsg.o comm/tcmi_transaction.o comm/tcmi_msg_factory.o \
	comm/tcmi_skel_msg.o comm/tcmi_skelresp_msg.o comm/tcmi_skel_procmsg.o comm/tcmi_skelresp_procmsg.o \
	comm/tcmi_err_msg.o comm/tcmi_err_procmsg.o comm/tcmi_vfork_done_procmsg.o comm/tcmi_exit_procmsg.o \
	comm/tcmi_p_emigrate_msg.o comm/tcmi_guest_started_procmsg.o comm/tcmi_ppm_p_migr_back_guestreq_procmsg.o \
	comm/tcmi_ppm_p_migr_back_shadowreq_procmsg.o comm/tcmi_rpc_procmsg.o comm/tcmi_rpcresp_procmsg.o \
	comm/tcmi_authenticate_msg.o comm/tcmi_authenticate_resp_msg.o comm/tcmi_signal_msg.o \
	comm/tcmi_generic_user_msg.o comm/tcmi_disconnect_msg.o

