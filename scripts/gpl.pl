#!/usr/bin/perl
###################################################################
#                            gpl.pl                               #
#                                                                 #
# Description: Applies a GPL Copyright to a specified file        #
#                                                                 #
#                                                                 # 
# Date: 05/08/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: gpl.pl,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $
#                                                                 #
###################################################################

use File::Basename;

# extract copyleft file name and file that the copyleft is to be
# applied to
($copyleftfile, $file) = @ARGV;

$filenew = "$file.new";

open(FH, "< $copyleftfile") or die "Opening $!";
@copyleft = <FH>;
close(FH);

open(OLD, "< $file") or die "Can't open $!";
open(NEW, "> $filenew") or die "Can't open $!";


print "Applying '", basename($copyleftfile),"' to '$file'..";
while(<OLD>) {
    if ($_ =~ /(.*)License\.\.\.\..*/) {
	    foreach $line (@copyleft) {
		print NEW "$1$line";
	    }
	}
    else {
	    print NEW $_;
	}
}
close(OLD);
close(NEW);
rename($file, "$file.orig");
rename($filenew, $file);

print "done.\n";



