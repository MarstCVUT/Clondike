#!/bin/sh
# migrates a process with a given PID home
# Synopsis:
#
# mig-home.sh PID [initiator]
# Initiator specifies where the initial migration home
# originates from (CCN or PEN)
#


# PID
pid=$1
# initiator defaults to pen
initiator=${2:-pen}

# synopsis
[  x$1 = x ] && {
    echo "Synopsis:"
    echo `basename $0` pid [initiator]
    echo Initiator specifies where the initial migration home
    echo originates from \(CCN or PEN\)
    exit 1
}

# check if pid valid
ps $pid > /dev/null
[ $? -ne 0 ] && {
    echo No such process: $pid
    exit $?
}

echo Migrating process:$pid home from $initiator
echo $pid  > /clondike/$initiator/mig/migrate-home
