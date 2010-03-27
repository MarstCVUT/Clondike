#!/bin/sh
###################################################################
#                          kkc_gpl.sh                             #
#                                                                 #
# Description: Applies a GPL Copyright to a file in Clondike      #
#                                                                 #
#                                                                 # 
# Date: 05/09/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id
#                                                                 #
###################################################################

scriptdir=`dirname $0`

gpl=$scriptdir/copyleftnote_clondike

file=$1;
while shift; do
    $scriptdir/gpl.pl $gpl $file
    file=$1;
done

