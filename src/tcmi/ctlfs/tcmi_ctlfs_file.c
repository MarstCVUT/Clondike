/**
 * @file tcmi_ctlfs_file.c - Definition of a class that represents file
 *                          class in tcmifs. This class extends tcmi_ctlfs_entry
 * 
 * 
 * 
 * 
 * 
 * 
 *
 * Date: 03/28/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ctlfs_file.c,v 1.3 2007/09/02 13:23:43 stavam2 Exp $
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

#include <linux/module.h>
#include <linux/errno.h>

#define TCMI_CTLFS_FILE_PRIVATE
#include "tcmi_ctlfs_file.h"

#include <dbg.h>


/** 
 * \<\<public\>\> Creates a new instance of a file. The file will be
 * used as an interface of the registered object to the user space. It
 * allows calling object methods and pass integer numbers as arguments
 * in ascii form.
 * 
 * For conversion ascii<->integer, the proc_dointvec() function is used from
 * kernel/sysctl.c
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param *object - points to the object that creates the file
 * @param *read_method - method of the file creator that will be called upon file read
 * @param *write_method - method of the file creator that will be called upon file write
 * @param maxlen - maximum length of the data in bytes
 * @param namefmt - nameformat string (printf style)
 * @return pointer to the new file or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_intfile_new(struct tcmi_ctlfs_entry *parent,
						mode_t mode,
						void *object,
						object_method *read_method,
						object_method *write_method,
						int maxlen,
						const char namefmt[], ...)

{
	struct tcmi_ctlfs_entry *file;
	va_list args;
	va_start(args, namefmt);
	file = tcmi_ctlfs_genericfile_new(parent, mode, object,
					  read_method, write_method,
					  maxlen, proc_dointvec, namefmt, args);
	va_end(args);
	return file;
}

/** 
 * \<\<public\>\> Creates a new instance of a file. The file will be
 * used as an interface of the registered object to the user space. It
 * allows calling object methods and pass strings as an argument.
 * 
 * For conversion overflow checking and newlines conversions are
 * handled by proc_dostring(). The function is used from
 * kernel/sysctl.c
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param *object - points to the object that creates the file
 * @param *read_method - method of the file creator that will be called upon file read
 * @param *write_method - method of the file creator that will be called upon file write
 * @param maxlen - maximum length of the data in bytes
 * @param namefmt - nameformat string (printf style)
 * @return pointer to the new file or NULL
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_strfile_new(struct tcmi_ctlfs_entry *parent,
					       mode_t mode,
					       void *object,
					       object_method *read_method,
					       object_method *write_method,
					       int maxlen,
					       const char namefmt[], ...)
{
	struct tcmi_ctlfs_entry *file;
	va_list args;
	va_start(args, namefmt);
	file = tcmi_ctlfs_genericfile_new(parent, mode, object,
					  read_method, write_method,
					  maxlen, proc_dostring, namefmt, args);
	va_end(args);
	return file;
}

/** 
 * \<\<public\>\> Creates a raw file instance.  The file will be used
 * as an interface of the registered object to the user space. It
 * allows calling object methods and passing raw unprocessed data as
 * an argument.
 * 
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param *object - points to the object that creates the file
 * @param *read_method - method of the file creator that will be called upon file read
 * @param *write_method - method of the file creator that will be called upon file write
 * @param maxlen - maximum length of the data in bytes
 * @param namefmt - nameformat string (printf style)
 * @return pointer to the new file or NULL
 * @note Currently not implemented
 */
struct tcmi_ctlfs_entry* tcmi_ctlfs_rawfile_new(struct tcmi_ctlfs_entry *parent,
						mode_t mode,
						void *object,
						object_method *read_method,
						object_method *write_method,
						int maxlen,
						const char namefmt[], ...)
{
	return NULL;
}


/** 
 * \<\<public\>\> This method unregisters the file uses the semaphore
 * lock as protection from races. This is the prefered way of
 * detaching the file from an object. If the file is to unregister
 * itself (e.g. from inside the registered object's methods), the
 * previous version is recommended to prevent deadlocks.
 *
 * @param *self - pointer to this file instance
 */
void tcmi_ctlfs_file_unregister(struct tcmi_ctlfs_entry *self) 
{
	struct tcmi_ctlfs_file *self_f = TCMI_CTLFS_FILE(self);
	if (self_f) {
		tcmi_ctlfs_file_lock(self_f);
		__tcmi_ctlfs_file_unregister(self);
		tcmi_ctlfs_file_unlock(self_f);

	}
}


/** 
 * \<\<public\>\> This method unregisters the file and allows the user
 * to specify the preferred way of unregistration (w/ or w/o the
 * lock). It is typically used if owning object uses this file is for
 * unregistering other files including this file.  All work is
 * delegate to either __tcmi_ctlfs_file_unregister() or
 * tcmi_ctlfs_file_unregister()
 *
 * @param *self - pointer to this file instance
 * @param unreg - describes a unregistration type, if a regular shutdown
 * is requested, the file is unregistered using the lock version. A file
 * unregistrationn from inside the registered object's method requires using
 * a lock free version. The access to the file is already serialized.
 */
void tcmi_ctlfs_file_unregister2(struct tcmi_ctlfs_entry *self, 
				 tcmi_ctlfs_file_unreg_t unreg) 
{
	/* unregistration issued from owning object's method, no locking */
	if (unreg == TCMI_CTLFS_FILE_FROM_METHOD)
		__tcmi_ctlfs_file_unregister(self);
	/* default case - explicit unregistration , requires locking*/
	else
		tcmi_ctlfs_file_unregister(self);
}

/** @addtogroup tcmi_ctlfs_file_class
 *
 * @{
 */

/** 
 * \<\<private\>\> This is a generic method, that creates a new
 * instance of a file as follows:
 *
 * - allocates a new instance and delegates the entry initialization to
 * the parent class. There is a custom tcmi_ctlfs_ops vector as file
 * destruction needs special handling. There are also file and inode
 * specific operations.
 * - allocates file associated data
 * - registers the object and its 2 methods with the file. None of
 * the methods are mandatory
 * - there is a custom entry_free operation that releases the 
 * above allocated file data upon file destruction
 * - proc_handler is associated with the data, so that ascii data
 * can be parsed when passing from/to the object's read/write method
 * 
 *
 * @param *parent - pointer to the parent entry - has to be a directory
 * @param *mode - access rights for the inode
 * @param *object - points to the object that creates the file
 * @param *read_method - method of the file creator that will be called upon file read
 * @param *write_method - method of the file creator that will be called upon file write
 * @param maxlen - maximum length of the data in bytes
 * @param *proc_handler - data parsing handler that does the conversion from/to ascii
 * @param namefmt - nameformat string (printf style)
 * @param args - arguments to the format string
 * @return pointer to the new file or NULL
 */
static struct tcmi_ctlfs_entry* tcmi_ctlfs_genericfile_new(struct tcmi_ctlfs_entry *parent,
							  mode_t mode,
							  void *object,
							  object_method *read_method,
							  object_method *write_method,
							  int maxlen,
							  proc_handler *proc_handler,
							  const char namefmt[], va_list args)
{
	struct tcmi_ctlfs_file *file;
	mdbg(INFO4, "Creating new file");

	if (!(file = kmalloc(sizeof(struct tcmi_ctlfs_file), GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate memory for file");
		goto exit0;
	}
	if (tcmi_ctlfs_entry_init(TCMI_CTLFS_ENTRY(file), 
				  TCMI_CTLFS_ENTRY(parent), 
				  S_IFREG | mode, 
				  &tcmi_ctlfs_file_ops, 
				  &tcmi_ctlfs_file_inode_operations,
				  &tcmi_ctlfs_file_operations, namefmt, args)) {
		mdbg(ERR3, "Failed to create the file");
		goto exit1;
	}
	if (!(file->data = kmalloc(maxlen, GFP_ATOMIC))) {
		mdbg(ERR3, "Failed to allocate %d bytes of file data", maxlen);
		goto exit2;
	}
	file->maxlen = maxlen;
	init_MUTEX(&file->f_sem);
	/* object associated with the file */
	file->object = object;
	file->read_method = read_method; /*TESTING */
	file->write_method = write_method; /*TESTING */
	file->proc_handler = proc_handler; /* TESTING */
	/* indicates a valid object has registered with the file */
	file->unregistered = 0; 
	return TCMI_CTLFS_ENTRY(file);

	/* error handling */
 exit2:
	kfree(file->data);   
 exit1:
	kfree(file);
 exit0:
	return NULL;
}

/** 
 * \<\<private\>\> This method is used to free the data associated with
 * the file. No explicit locking is needed since:
 * - the semaphore is already held by the unregister function or
 * - the method of the registered object is currently executing and the file
 * protects itself and this is an attempt to destroy the file from inside the
 * object method.
 * 
 * @param *entry - points to the file that is to be freed
 * @return 0 upon succes.
 */
static void tcmi_ctlfs_file_release(struct tcmi_ctlfs_entry *entry)
{
	struct tcmi_ctlfs_file *file = TCMI_CTLFS_FILE(entry);
	
	mdbg(INFO4, "Freeing file associated data(%p)", file->data);
	kfree(file->data);
	file->data = NULL;

}


/** 
 * \<\<private\>\> The read file operation, called by the read system call. 
 * The access to the file associated data is serialized, so that the
 * object method always receives consistent data.
 * - Read requests are possible only from offset (ppos) 0.
 * - Initial check whether an object is already unregistered with the file is 
 *   performed. This is to prevent calling the method when the the object is
 *   currently detaching from the file. The associated object might have already 
 *   taken steps that don't allow calling the read method - E.g. has already
 *   woken up the sleeping process and doesn't want another process to call
 *   the method again.
 * - The registered object is first sent a read request and data 
 *   is returned
 * - The data is then processed by the proc_handler. It is necessary 
 *   to setup the ctl_table, so that the we can reuse the sysctl.c
 *   data parsing code for integers and strings.
 *
 * @param *filp - pointer to the kernel file object that is being read
 * @param *buf - user buffer where to store the result
 * @param count - number of bytes to be read
 * @param *ppos - pointer to the current position in the file
 * @return - when >= 0 - number of bytes written, the result from the proc_handler
 * is usually 0, just to denote success converting from binary to ascii
 *           when <  0 - error
 */
static ssize_t tcmi_ctlfs_file_read(struct file *filp, char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct tcmi_ctlfs_file *file = 
		TCMI_CTLFS_FILE(TCMI_CTLFS_DENTRY_TO_ENTRY(filp->f_dentry));
	size_t res;
	ssize_t error = 0;
	
	struct ctl_table tbl = {
		.data = file->data,
		.maxlen = file->maxlen,
	};
	mdbg(INFO4, "file read in progress count=%lu, ppos=%Ld", (unsigned long)count, *ppos);
	if (tcmi_ctlfs_file_lock_interruptible(file)) {
		error = -ERESTARTSYS;
		goto exit0;
	}
	/* when object unregistering is in progress don't start any operation */
	if (file->unregistered)
		goto exit1;
	/* when trying from other position than 0, reset the file position and quit */
	if (*ppos) {
		*ppos = 0;
		goto exit1;
	}
	if (file->read_method)
		if ((error = file->read_method(file->object, file->data)))
			goto exit1;
	if (file->proc_handler) {
		/* res is in/out parameter, initially stores the number of requested bytes.
		 The handler then returns the number of bytes really processed */
		res = count;
		if ((error = file->proc_handler(&tbl, 0, buf, &res, ppos)))
			goto exit1;
		error = (ssize_t)res;
	}

 exit1:
	tcmi_ctlfs_file_unlock(file);
	/* signal arrived, semaphore still busy */
 exit0: 
	return error;
}

/**
 * \<\<private\>\> The write file operation, called by the write system call. 
 * The access to the file associated data is serialized, so that the
 * object method always receives consistent data.
 * - Initial check whether an object is already unregistered with the file is 
 *   performed. This is to prevent calling the method when the the object is
 *   currently detaching from the file. The associated object might have already 
 *   taken steps that don't allow calling the write method - E.g. has already
 *   woken up the sleeping process and doesn't want another process to call
 *   the method again.
 * - The requested file offset can be arbitrary, but must not exceed
 *   the size of the data assoicated with the file - handled by the 
 *   proc_handler
 * - The data is first processed by the proc_handler. It is necessary
 *   to setup the ctl_table, so that the we can reuse the sysctl.c
 *   data parsing code for integers and strings.
 * - Finally, the registered object is notified about the write event
 *   and the data is passed to it.
 *
 * @param *filp - pointer to the kernel file object that is being written
 * @param *buf - user buffer where to take the source data
 * @param count - number of bytes to be written
 * @param *ppos - pointer to the current position in the file
 * @return - when >= 0 - number of bytes written, the result from the proc_handler
 * is usually 0, just to denote success converting the ascii data to binary
 *           when <  0 - error
 */
static ssize_t tcmi_ctlfs_file_write(struct file *filp, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct tcmi_ctlfs_file *file = 
		TCMI_CTLFS_FILE(TCMI_CTLFS_DENTRY_TO_ENTRY(filp->f_dentry));
	size_t res;
	ssize_t error = 0;

	struct ctl_table tbl = {
		.data = file->data,
		.maxlen = file->maxlen,
	};
	mdbg(INFO4, "file write in progress count=%lu, ppos=%Ld", (unsigned long)count, *ppos);
	if (tcmi_ctlfs_file_lock_interruptible(file)) {
		error = -ERESTARTSYS;
		goto exit0;
	}
	/* when object unregistering is in progress don't start any operation */
	if (file->unregistered)
		goto exit1;
	if (file->proc_handler) {
		res = count;
		if ((error = file->proc_handler(&tbl, 1, (char __user *)buf, &res, ppos)))
			goto exit1;
	}
	if (file->write_method) {
		if ((error = file->write_method(file->object, file->data)))
			goto exit1;
		error = (ssize_t)res;
	}
 exit1:
	tcmi_ctlfs_file_unlock(file);
	/* signal arrived, semaphore still busy */
 exit0: 
	return error;
}

/** 
 * This method is used when object associated with the file wants to
 * unregister with the file.  The calling object is responsible for waking 
 * up a user process currently using this file (e.g. that have it open
 * and might be sleeping in the read/write methods of the object). 
 *
 * This method assumes, that the file lock is already held by the
 * caller. A typical use can be when the file is supposed to destroy
 * itself upon user reading from or writing into the file (from inside
 * the registered object's methods).
 *
 * The unregistration requires resetting read/write methods and
 * associated proc_handler and object pointer. The unregistered flag
 * ensures that the methods will be ignored on the next read/write issued
 * by any user process. 
 *
 * @param *self - pointer to this file instance
 */
static void __tcmi_ctlfs_file_unregister(struct tcmi_ctlfs_entry *self) 
{
	struct tcmi_ctlfs_file *self_f = TCMI_CTLFS_FILE(self);
	if (self_f) {
		/* in case the object hasn't set the unregistered flag in advance */
		tcmi_ctlfs_file_unregister_prepare(self);
		self_f->object = NULL;
		self_f->read_method = NULL;
		self_f->write_method = NULL;
		self_f->proc_handler = NULL;
	}
}

/** A file requires special entry operations to free all resources. */
static struct tcmi_ctlfs_ops tcmi_ctlfs_file_ops = {
	.entry_release = tcmi_ctlfs_file_release,
}; 
/** File operations for VFS. */
static struct file_operations tcmi_ctlfs_file_operations = {
	.read		= tcmi_ctlfs_file_read,
	.write		= tcmi_ctlfs_file_write,
};

/** Inode operations for VFS, the only method getattr is taken from fs/libfs\.c. */
static struct inode_operations tcmi_ctlfs_file_inode_operations = {
	.getattr	= simple_getattr,
};

/**
 * @}
 */


EXPORT_SYMBOL_GPL(tcmi_ctlfs_intfile_new);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_strfile_new);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_rawfile_new);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_file_unregister);
EXPORT_SYMBOL_GPL(tcmi_ctlfs_file_unregister2);
