#!/bin/sh
###################################################################
#                          kkc_gpl.sh                             #
#                                                                 #
# Description: Applies a GPL Copyright to a file in KKC           #
#                                                                 #
#                                                                 # 
# Date: 05/08/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: kkc_gpl.sh,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $
#                                                                 #
###################################################################

scriptdir=`dirname $0`

gpl=$scriptdir/copyleftnote_kkc

file=$1;
while shift; do
    $scriptdir/gpl.pl $gpl $file
    file=$1;
done

