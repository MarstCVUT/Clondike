#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: $0 mount-ip compilation-dir"	
	exit 1
fi

MOUNT_IP=$1
COMPILATION_DIR=$2


mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/ /mnt/clondike/${MOUNT_IP}-0-0/usr/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/lib /mnt/clondike/${MOUNT_IP}-0-0/lib 
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/home/clondike/ /mnt/clondike/${MOUNT_IP}-0-0/home/clondike/ 
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/etc/ /mnt/clondike/${MOUNT_IP}-0-0/etc/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/bin/ /mnt/clondike/${MOUNT_IP}-0-0/bin/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/sbin/ /mnt/clondike/${MOUNT_IP}-0-0/sbin/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/${COMPILATION_DIR} /mnt/clondike/${MOUNT_IP}-0-0/${COMPILATION_DIR} -o cache_filter=*.c:*.h:*.cc
