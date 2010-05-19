##############################################################################
# @file Makefile - Builds Control Filesystem
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

obj-$(CONFIG_TCMI) := tcmictlfs.o
tcmictlfs-objs     := tcmi_ctlfs.o tcmi_ctlfs_entry.o tcmi_ctlfs_dir.o \
		      tcmi_ctlfs_file.o tcmi_ctlfs_symlink.o  
