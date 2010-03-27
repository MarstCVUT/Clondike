#!/bin/sh
###################################################################
#                          tcmi_gpl.sh                            #
#                                                                 #
# Description: Applies a GPL Copyright to a file in TCMI          #
#                                                                 #
#                                                                 # 
# Date: 05/08/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: tcmi_gpl.sh,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $
#                                                                 #
###################################################################

scriptdir=`dirname $0`

gpl=$scriptdir/copyleftnote_tcmi

file=$1;
while shift; do
    $scriptdir/gpl.pl $gpl $file
    file=$1;
done

