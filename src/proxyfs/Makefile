##############################################################################
# @file Makefile - Builds ProxyFS
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

obj-$(CONFIG_TCMI) := proxyfs.o
proxyfs-objs += proxyfs_module.o proxyfs_fs.o proxyfs_msg.o proxyfs_peer.o buffer.o proxyfs_ioctl_table_sorted.o \
		proxyfs_real_file.o proxyfs_tty_real_file.o proxyfs_generic_real_file.o proxyfs_pipe_real_file.o \
		proxyfs_proxy_file.o proxyfs_pipe_proxy_file.o proxyfs_generic_proxy_file.o \
		proxyfs_file.o proxyfs_task.o proxyfs_client.o proxyfs_server.o 

$(obj)/proxyfs_ioctl_table_sorted.s: $(src)/proxyfs_ioctl_table.s
	proxyfs_ioctl_sort.pl < $< > $@
$(obj)/proxyfs_ioctl_table.s: $(src)/proxyfs_ioctl_table.c
$(obj)/proxyfs_ioctl_table_sorted.o: $(src)/proxyfs_ioctl_table_sorted.s
	$(CC) -c $< -o $@


