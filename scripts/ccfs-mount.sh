#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Usage: $0 mount-ip compilation-dir"	
	exit 1
fi

MOUNT_IP=$1
COMPILATION_DIR=$2

mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/local /mnt/clondike/${MOUNT_IP}-0-0/usr/local
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/lib  /mnt/clondike/${MOUNT_IP}-0-0/usr/lib
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/lib64 /mnt/clondike/${MOUNT_IP}-0-0/usr/lib64
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/lib32 /mnt/clondike/${MOUNT_IP}-0-0/usr/lib32
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/share /mnt/clondike/${MOUNT_IP}-0-0/usr/share
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/bin /mnt/clondike/${MOUNT_IP}-0-0/usr/bin
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/usr/sbin /mnt/clondike/${MOUNT_IP}-0-0/usr/sbin

mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/lib /mnt/clondike/${MOUNT_IP}-0-0/lib 
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/home/clondike/ /mnt/clondike/${MOUNT_IP}-0-0/home/clondike/ 
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/etc/ /mnt/clondike/${MOUNT_IP}-0-0/etc/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/bin/ /mnt/clondike/${MOUNT_IP}-0-0/bin/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/sbin/ /mnt/clondike/${MOUNT_IP}-0-0/sbin/
mount -t ccfs /mnt/clondike/${MOUNT_IP}-0-0/${COMPILATION_DIR} /mnt/clondike/${MOUNT_IP}-0-0/${COMPILATION_DIR} -o cache_filter=*.c:*.h:*.cc
