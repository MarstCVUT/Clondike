#!/bin/sh
###################################################################
#                       install.sh                                #
#                                                                 #
# Description: installation steps include:                        #
#              - creating new user 'clondike' w/ uid=999          #
#              - creating ctlfs mount point at /clondike          #
#                                                                 #
#                                                                 #
# Synopsis:                                                       #
# install.sh                                                      #
#                                                                 #
# Date: 05/15/2005                                                #
#                                                                 #
# Author: Jan Capek                                               #
#                                                                 #
# $Id: install.sh,v 1.2 2008/05/02 19:59:20 stavam2 Exp $
#                                                                 #
###################################################################


CLONDIKEUSER=clondike
CLONDIKEUID=999
CLONDIKEDIR=/clondike
CLONDIKEMNT=/mnt/clondike
CLONDIKEHOME=/home/clondike
PROXYDIR=/mnt/proxy
LOCALPROCDIR=/mnt/local/proc

useradd --uid $CLONDIKEUID $CLONDIKEUSER  && {
    echo "Created new user '$CLONDIKEUSER', uid=$CLONDIKEUID."
}

[ ! -d $CLONDIKEDIR ] && {
    mkdir $CLONDIKEDIR
    echo "Created ctlfs directory '$CLONDIKEDIR'."
}

[ ! -d $CLONDIKEMNT ] && {
    mkdir $CLONDIKEMNT
    echo "Created mounting directory '$CLONDIKEMNT'."
}

[ ! -d $CLONDIKEHOME ] && {
    mkdir $CLONDIKEHOME
    echo "Created home directory '$CLONDIKEHOME'."
}

[ ! -d $PROXYDIR ] && {
    mkdir $PROXYDIR
    echo "Created proxy directory '$PROXYDIR'."
}

[ ! -d $LOCALPROCDIR ] && {
    mkdir -p $LOCALPROCDIR
    echo "Created local proc directory '$LOCALPROCDIR'."
}

