##############################################################################
# @file debug.make - Sets up per directory verbosity level		  
#                    0 - messages are disabled
#                         ...                                
#                    4 - all messages are printed
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

export MDBG_ccfs_CRIT := 0
export MDBG_ccfs_ERR  := 4
export MDBG_ccfs_WARN := 0
export MDBG_ccfs_INFO := 0

export MDBG_kkc_CRIT := 0
export MDBG_kkc_ERR  := 4
export MDBG_kkc_WARN := 0
export MDBG_kkc_INFO := 0

export MDBG_proxyfs_CRIT := 4
export MDBG_proxyfs_ERR  := 4
export MDBG_proxyfs_WARN := 0
export MDBG_proxyfs_INFO := 0

export MDBG_arch_CRIT := 0
export MDBG_arch_ERR  := 4
export MDBG_arch_WARN := 0
export MDBG_arch_INFO := 0

# Director and director netlink
export MDBG_director_CRIT := 4
export MDBG_director_ERR  := 4
export MDBG_director_WARN := 4
export MDBG_director_INFO := 4

export MDBG_netlink_CRIT := 4
export MDBG_netlink_ERR  := 4
export MDBG_netlink_WARN := 4
export MDBG_netlink_INFO := 4

# TCMI
export MDBG_comm_CRIT := 4
export MDBG_comm_ERR  := 4
export MDBG_comm_WARN := 4
export MDBG_comm_INFO := 4

export MDBG_ckpt_CRIT := 4
export MDBG_ckpt_ERR  := 4
export MDBG_ckpt_WARN := 4
export MDBG_ckpt_INFO := 4

export MDBG_task_CRIT := 4
export MDBG_task_ERR  := 4
export MDBG_task_WARN := 4
export MDBG_task_INFO := 4

export MDBG_manager_CRIT := 4
export MDBG_manager_ERR  := 4
export MDBG_manager_WARN := 4
export MDBG_manager_INFO := 4

export MDBG_ctlfs_CRIT := 0
export MDBG_ctlfs_ERR  := 4
export MDBG_ctlfs_WARN := 0
export MDBG_ctlfs_INFO := 0

export MDBG_syscall_CRIT := 4
export MDBG_syscall_ERR  := 4
export MDBG_syscall_WARN := 4
export MDBG_syscall_INFO := 4

export MDBG_lib_CRIT := 4
export MDBG_lib_ERR  := 4
export MDBG_lib_WARN := 4
export MDBG_lib_INFO := 4

export MDBG_tcmi_CRIT := 4
export MDBG_tcmi_ERR  := 4
export MDBG_tcmi_WARN := 4
export MDBG_tcmi_INFO := 4

export MDBG_migration_CRIT := 4
export MDBG_migration_ERR  := 4
export MDBG_migration_WARN := 4
export MDBG_migration_INFO := 4

export MDBG_fs_CRIT := 4
export MDBG_fs_ERR  := 4
export MDBG_fs_WARN := 4
export MDBG_fs_INFO := 4

export MDBG_current_CRIT := 4
export MDBG_current_ERR  := 4
export MDBG_current_WARN := 4
export MDBG_current_INFO := 4
