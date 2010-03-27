#!/usr/bin/perl
my %asm_array;
while(<>){
	print;
	/^ioctl_sizes:/ && do {
	       	while(<>){
			($a) = /.long\t([0-9]+)/ or last;
			($asm_array{$a}) = <> =~ /.long\t([0-9]+)/;
		}	
		print "\t.long\t", $_, "\n\t.long\t", $asm_array{$_}, "\n" foreach( sort keys %asm_array );
		print;
	 };
}
