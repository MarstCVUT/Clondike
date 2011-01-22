#!/bin/sh

# Uploads compiled clondike binaries and ruby director files onto a remote host
# The structure of Clondike dirs is assumed to be exactly the same as the development build structure.
# NOTE: This script does NOT actually install Clondike on a remote system, it already assumes Clondike is installed there and it just updates binaries. It is more used for developing, rather than for a "client usage"

HOST=$1
CLONDIKE_ROOT=$2
OPTS=

scp $OPTS ../src/kkc/kkc_lib.ko  $HOST:$CLONDIKE_ROOT/src/kkc/
scp $OPTS ../src/director/director-mod.ko $HOST:$CLONDIKE_ROOT/src/director/
scp $OPTS ../src/proxyfs/proxyfs.ko $HOST:$CLONDIKE_ROOT/src/proxyfs/
scp $OPTS ../src/tcmi/ctlfs/tcmictlfs.ko  $HOST:$CLONDIKE_ROOT/src/tcmi/ctlfs/
scp $OPTS ../src/tcmi/ckpt/tcmickptcom.ko  $HOST:$CLONDIKE_ROOT/src/tcmi/ckpt/
scp $OPTS ../src/tcmi/tcmi.ko  $HOST:$CLONDIKE_ROOT/src/tcmi/
scp $OPTS ../src/ccfs/ccfs.ko $HOST:$CLONDIKE_ROOT/src/ccfs/

echo "Kernel modules uploaded"

scp -r $OPTS ../userspace/simple-ruby-director/*.rb $HOST:$CLONDIKE_ROOT/userspace/simple-ruby-director
scp -r $OPTS ../userspace/simple-ruby-director/trust/*.rb $HOST:$CLONDIKE_ROOT/userspace/simple-ruby-director/trust
scp -r $OPTS ../userspace/simple-ruby-director/cli/*.rb $HOST:$CLONDIKE_ROOT/userspace/simple-ruby-director/cli
scp -r $OPTS ../userspace/simple-ruby-director/measure/*.rb $HOST:$CLONDIKE_ROOT/userspace/simple-ruby-director/measure
scp -r $OPTS ../userspace/simple-ruby-director/*.so $HOST:$CLONDIKE_ROOT/userspace/simple-ruby-director

echo "Ruby files uploaded"

