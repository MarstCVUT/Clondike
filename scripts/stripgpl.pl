#!/usr/bin/perl
###################################################################
#                        stripgpl.pl                              #
#                                                                 #
# Description: Strips a GPL Copyright from a specified file       #
#                                                                 #
#                                                                 # 
# Date: 05/08/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: stripgpl.pl,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $
#                                                                 #
###################################################################

use File::Basename;

$start_copyleft = "This file is part of";
$end_copyleft = "Foundation, Inc";

foreach $file (@ARGV) {
    $filenew = "$file.new";
    $inlicense = 0; # not in license section
    open(OLD, "< $file") or die "Can't open $!";
    open(NEW, "> $filenew") or die "Can't open $!";
    print "Stripping GPL from '$file'";
    while(<OLD>) {
	if ($_ =~ /(.*)This file is part of.*/) {
		print NEW "$1License....\n";
		$inlicense = 1;
	    }
	print NEW $_ if ($inlicense == 0);
	$inlicense = 0 if ($_ =~ /.*Foundation, Inc.*/);
    }
    close(OLD);
    close(NEW);
    rename($file, "$file.orig");
    rename($filenew, $file);
    print "done.\n";
}





