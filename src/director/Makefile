##############################################################################
# @file Makefile - Builds director
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

obj-$(CONFIG_TCMI) := director-mod.o
director-mod-objs  := director.o netlink/genl_ext.o netlink/npm_msg.o \
		      netlink/node_connected_msg.o netlink/node_disconnected_msg.o netlink/task_exitted_msg.o \
		      netlink/immigration_request_msg.o netlink/generic_user_message_recv_msg.o \
		      netlink/emigration_failed_msg.o netlink/migrated_home_msg.o netlink/immigration_confirmed_msg.o\
		      netlink/task_forked_msg.o netlink/generic_user_message_send_handler.o netlink/comm.o

