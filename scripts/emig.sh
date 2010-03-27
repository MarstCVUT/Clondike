#!/bin/sh
###################################################################
#                          emig.sh                                #
#                                                                 #
# Description: Emigrates a process with a given PID to a PEN      #
#              using a specified method                           #
#                                                                 #
# Synopsis:                                                       #
# emig.sh pid [pen [method]]                                      #
#                                                                 #
# pen defaults to 0, method defaults to PPM_P                     #
# (preemptive migration w/ physical checkpoint)                   #
#                                                                 # 
# Date: 05/07/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: emig.sh,v 1.1.1.1 2007/03/01 14:16:10 xkacer Exp $                                                            #
#                                                                 #
###################################################################


# root CCN directory
ccndir=/clondike/ccn

# process pid
pid=$1
# set default PEN ID
pen=${2:-0}
method=${3:-ppm-p}
migfile=$ccndir/mig/emigrate-$method

# check if PID has been specified
[  x$1 = x ] && {
    echo "Synopsis:"
    echo `basename $0` pid [pen [method]]
    echo Available methods: 'ppm-p' 
    exit 1
}
# check if pid id valid
ps $pid > /dev/null
[ $? -ne 0 ] && {
    echo No such process: $pid
    exit $?
}

# check if migration method is correct
[ ! -w $migfile ] && {
    echo Wrong migration method: $method
    exit 1
}

#check if PEN with given ID is connected
[ ! -r $ccndir/nodes/$pen ] && {
    echo Invalid PEN ID: $pen
    exit 1
}

echo Migrating process: $pid to PEN: $pen method: $method
echo $pid $pen  > /clondike/ccn/mig/emigrate-$method
