##############################################################################
# @file Makefile - Builds Checkpointing component
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

obj-$(CONFIG_TCMI) := tcmickptcom.o
tcmickptcom-objs   := tcmi_ckptcom.o tcmi_ckpt.o tcmi_ckpt_openfile.o \
		      tcmi_ckpt_vm_area.o ../../arch/arch_ids.o \
		      ../../arch/current/regs.o

