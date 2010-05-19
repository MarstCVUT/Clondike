#!/usr/bin/perl
##############################################################################
# @file proxyfs_ioctl_sort.pl - Sorts ioctl table
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

my %asm_array;
while(<>){
	print;
	/^ioctl_table:/ && do {
	       	while(<>){
			($a) = /.long\t(-?[0-9]+)/ or last;
			($asm_array{$a}) = <> =~ /.long\t(-?[0-9]+)/;
		}	
		print "\t.long\t", $_, "\n\t.long\t", $asm_array{$_}, "\n" foreach( sort keys %asm_array );
		print;
	 };
}
