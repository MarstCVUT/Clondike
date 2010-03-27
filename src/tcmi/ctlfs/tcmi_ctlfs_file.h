/**
 * @file tcmi_ctlfs_file.h - Declaration of a class that represents file
 *                          class in tcmifs. This class extends tcmi_ctlfs_entry
 * 
 * 
 * 
 * 
 * 
 * 
 *
 * Date: 03/26/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_file.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef TCMI_CTLFS_FILE_H
#define TCMI_CTLFS_FILE_H

/* ctl_table and proc_do{int,string} are reused */
#include <linux/sysctl.h> 
#include <linux/semaphore.h>

#include "tcmi_ctlfs_dir.h"


/** describes the signature of a method that an object registers with a file */
typedef int object_method (void *object, void *data);

/** @defgroup tcmi_ctlfs_file_class tcmi_ctlfs_file class 
 * 
 * @ingroup tcmi_ctlfs_entry_class
 *
 * A file in the TCMI control file system is used to export object
 * interfaces.  An object or a component creates a new control file
 * for each method it wants to made available to the user space. The
 * design idea is an extension of how linux sysctl works.  
 *
 * - When a user space application reads from a file, the associated
 * object is notified by calling its registered method. The result is
 * communicated back to user space. Similarly to sysctl, there are
 * various data types that can be passed to the objects method. Their
 * conversion from/to ascii is handled by special handlers.
 *
 * - When a user space program writes data to the file, the data is
 * parsed by the TCMI file object and the binary form is passed on to
 * the registered object's method.
 *
 * As in sysctl, following functions for parsing ascii input data are provided:
 * - string
 * - an array of integers
 * - in addition, a raw data can be passed into the method of the registered object.
 *
 * Based on this we can create three types of files:
 * - integer files - methods of file instantiators accept/return integer data.
 *                   Conversion from/to ascii is handled internally by this class
 * - string files  - methods of file instantiators accept/return string data.
 *                   String parsing (overflow checks, new lines conversion) is 
 *                   handled internally by this class.
 * - raw files     - methods of file instantiators accept raw data.
 *                   No conversion is performed.
 *
 * The object that creates the control file, specifies maximum allowed
 * data length as it is in sysctl.
 * 
 * To prevent code duplication, this class reuses the procfs
 * functions(sysctl.c) - proc_dostring(), proc_dointvec() for ascii/binary
 * conversions.
 *
 * When the data is being passed to registered object's method, a
 * mutual access guarantees integrity (implemented using mutex)
 *
 * 
 * @{
 */

/** Describes the type of the file unregistration that is taking
 * place.
 *
 * - regular way - is explicit file unregistration without using the
 * file. This is requires locking the actual file, before
 * unregistering its object.
 * - from a method of the registered object - this is a special
 * situation when the file is already locked (access is serialized by
 * read/write file operations). Therefore, no locking is needed.
 */
typedef enum {
	TCMI_CTLFS_FILE_REGULAR,
	TCMI_CTLFS_FILE_FROM_METHOD
} tcmi_ctlfs_file_unreg_t;

/**
 * A file compound structure, extends the parent entry class.
 * Contains a reference to the object whose read/write methods
 * are to be called.
 */
struct tcmi_ctlfs_file {
	/** parent class instance. */
	struct tcmi_ctlfs_entry super;

	/** mutex semaphore to serialized the file access via
	 * read/write methods. */
	struct semaphore f_sem;

	/** object that instantiated this file. */
	void *object;
	/** read method that the file instantiator registers. */
	object_method *read_method;
	/** write method that the file instantiator registers. */
	object_method *write_method;
	/** data parsing handler (reused from sysctl). */
	proc_handler *proc_handler;

	/** storage for the data associated with the file. */
	void *data;
	/** maximal data length. */
	int maxlen;
	/** flag that indicates when the object unregisters with the file */
	int unregistered;
};

/** Casts an entry to file */
#define TCMI_CTLFS_FILE(e) ((struct tcmi_ctlfs_file *)e)

/** \<\<public\>\> Creates a new integer file instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_intfile_new(struct tcmi_ctlfs_entry *parent,
						      mode_t mode,
						      void *object,
						      object_method *read_method,
						      object_method *write_method,
						      int maxlen,
						      const char namefmt[], ...);

/** \<\<public\>\> Creates a new string file instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_strfile_new(struct tcmi_ctlfs_entry *parent,
						      mode_t mode,
						      void *object,
						      object_method *read_method,
						      object_method *write_method,
						      int maxlen,
						      const char namefmt[], ...);

/** \<\<public\>\> Creates a raw file instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_rawfile_new(struct tcmi_ctlfs_entry *parent,
						       mode_t mode,
						       void *object,
						       object_method *read_method,
						       object_method *write_method,
						       int maxlen,
						       const char namefmt[], ...);

/**
 * \<\<public\>\> Sets the unregistered flag of the file. This is used when the
 * associated object is about to unregister from the file. However,
 * the object needs to be guaranteed that from now on, no new process
 * will send him a read/write message. This is exactly what the
 * unregistered flag does.
 *
 * @param *self - pointer to this file instance
 * \sa tcmi_ctlfs_file_unregister()
 */
static inline void tcmi_ctlfs_file_unregister_prepare(struct tcmi_ctlfs_entry *self) 
{
	if (self)
		TCMI_CTLFS_FILE(self)->unregistered = 1;
}


/** \<\<public\>\> Unregisters the file. */
extern void tcmi_ctlfs_file_unregister(struct tcmi_ctlfs_entry *self);

/** \<\<public\>\> Unregisters the file, advanced version */
extern void tcmi_ctlfs_file_unregister2(struct tcmi_ctlfs_entry *self, 
					tcmi_ctlfs_file_unreg_t unreg);

/**
 * \<\<public\>\> Locks this file instance, interruptible version, the
 * process can be woken up by a signal.
 *
 * @param *self - pointer to this file instance
 */
static inline int tcmi_ctlfs_file_lock_interruptible(struct tcmi_ctlfs_file *self)
{
	return (down_interruptible(&self->f_sem));
}

/**
 * \<\<public\>\> Locks this file instance.
 *
 * @param *self - pointer to this file instance
 */
static inline void tcmi_ctlfs_file_lock(struct tcmi_ctlfs_file *self)
{
		down(&self->f_sem);
}

/**
 * \<\<public\>\> Unlocks this file instance.
 *
 * @param *self - pointer to this file instance
 */
static inline void tcmi_ctlfs_file_unlock(struct tcmi_ctlfs_file *self)
{
		up(&self->f_sem);
}



/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CTLFS_FILE_PRIVATE
/** A forward declaration, generic file constructor. */
static struct tcmi_ctlfs_entry* tcmi_ctlfs_genericfile_new(struct tcmi_ctlfs_entry *parent,
							   mode_t mode,
							   void *object,
							   object_method *read_method,
							   object_method *write_method,
							   int maxlen,
							   proc_handler *proc_handler,
							   const char namefmt[], va_list args);
/** A forward declaration, frees all file resources. */
static void tcmi_ctlfs_file_release(struct tcmi_ctlfs_entry *entry);



/** A forward declaration, read operation for VFS. */
static ssize_t tcmi_ctlfs_file_read(struct file *filp, char __user *buf,
				    size_t count, loff_t *ppos);
/** A forward declaration, write operation for VFS. */
static ssize_t tcmi_ctlfs_file_write(struct file *filp, const char __user *buf,
				     size_t count, loff_t *ppos);

/** Unregisters the associated object - lock free version */
static void __tcmi_ctlfs_file_unregister(struct tcmi_ctlfs_entry *self);

/** A forward declaration, special operations for tcmi_ctlfs entries. */
static struct tcmi_ctlfs_ops tcmi_ctlfs_file_ops;
/** A forward declaration, file operations for VFS. */
static struct file_operations tcmi_ctlfs_file_operations;
/** A forward declaration, inode operations for VFS. */
static struct inode_operations tcmi_ctlfs_file_inode_operations;

#endif /* TCMI_CTLFS_FILE_PRIVATE */

/**
 * @}
 */


#endif /* TCMI_CTLFS_FILE_H */

