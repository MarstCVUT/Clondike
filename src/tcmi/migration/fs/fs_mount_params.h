#ifndef _FS_MOUNT_PARAMS_H
#define _FS_MOUNT_PARAMS_H

/** Max length of the mount type name */
#define TCMI_FS_MOUNT_LENGTH 80
/** Max length of the mount options */
#define TCMI_FS_MOUNT_OPTIONS_LENGTH 128
/** Max length of the mount device */
#define TCMI_FS_MOUNT_DEVICE_LENGTH 64

/** Helper structure that keeps data about the mounting which should be performed on PEN */
struct fs_mount_params {
	/** Mounter to be used */
	char mount_type[TCMI_FS_MOUNT_LENGTH];
	/** Mounter options */
	char mount_options[TCMI_FS_MOUNT_OPTIONS_LENGTH];
	/** Mounter device */
	char mount_device[TCMI_FS_MOUNT_DEVICE_LENGTH];
} __attribute__((__packed__));;

#endif
