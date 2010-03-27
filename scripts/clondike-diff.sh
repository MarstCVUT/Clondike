#!/bin/sh
###################################################################
#                       clondike-diff.sh                          #
#                                                                 #
# Description: Creates diff of clondike related files             #
#              in two kernel trees                                #
# Synopsis:                                                       #
# clndike-diff origkernel/ clondikernel/                          #
#                                                                 #
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: clondike-diff.sh,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $                                                            #
#                                                                 #
###################################################################



src=/usr/src
origdir=${1:-linux-2.6.11.7}
newdir=${2:-linux-2.6.11.7-clondike}

# check if original directory exists
[ ! -d $src/$origdir ] && {
    echo Invalid directory: \'$1\' for original kernel
    exit 1
}

# check if original directory exists
[ ! -d $src/$newdir ] && {
    echo Invalid directory: \'$2\' for new kernel
    exit 1
}


FINDSTRING="-name '*.c' -o -name '*.h' -o -name 'Makefile' -o -name 'Kconfig'"

DIFF=`cd $src/$newdir; eval find clondike/ ${FINDSTRING}; eval find include/clondike ${FINDSTRING}`

DIFF="$DIFF include/linux/sched.h arch/i386/mm/fault.c mm/memory.c arch/i386/Kconfig  Makefile"
DIFF="$DIFF kernel/signal.c kernel/exit.c fs/exec.c"

cd $src
for f in $DIFF; do 
    echo Diffing: $f >&2
    diff -Naur $origdir/$f $newdir/$f;
done;
