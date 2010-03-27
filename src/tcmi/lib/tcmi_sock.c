/**
 * @file tcmi_sock.c - A implementation of wrapper class for KKC listenings.
 *                       
 *                      
 * 
 *
 *
 * Date: 04/17/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_sock.c,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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

#include <linux/string.h>

#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>

#define TCMI_SOCK_PRIVATE
#include "tcmi_sock.h"

#include <dbg.h>



/**
 * \<\<public\>\> TCMI socket constructor.
 * A listening is constructed as follows:
 * - a new instance is allocated
 * - a new socket directory is created in the root directory. The name
 * of the directory is specified by the method caller.
 * - localname, peername and archname control files are crated
 *
 * @param *root - directory which all listening control files and its
 * main directory will reside in.
 * @param *k_sock - KKC socket, that this instance is to present in ctlfs
 * @param namefmt - nameformat string (printf style)
 * @return - a new tcmi_sock instance or NULL
 */
extern struct tcmi_sock* tcmi_sock_new(struct tcmi_ctlfs_entry *root,
				       struct kkc_sock *k_sock,
				       const char namefmt[], ...)
{
	struct tcmi_sock *t_sock;
	va_list args;
	va_start(args, namefmt);
	mdbg(INFO3, "Creating new TCMI socket");

	if (!(t_sock = kmalloc(sizeof(struct tcmi_sock), GFP_KERNEL))) {
		mdbg(ERR3, "Failed to allocate memory for tcmi socket '%s'",
		     kkc_sock_getsockname2(k_sock));
		goto exit0;
	}
	/* create KKC t_sock */
	atomic_set(&t_sock->ref_count, 1);

	t_sock->sock = kkc_sock_get(k_sock);

	/* main t_sock directory named after the index of its slot */
	if (!(t_sock->d_id = 
	      tcmi_ctlfs_dir_vnew(root, TCMI_PERMS_DIR, namefmt, args)))
		goto exit1;
	if (!(t_sock->f_sockname = 
	      tcmi_ctlfs_strfile_new(t_sock->d_id, TCMI_PERMS_FILE_R,
				     t_sock, tcmi_sock_getsockname, NULL,
				     KKC_SOCK_MAX_ADDR_LENGTH, "sockname")))
		goto exit2;
	if (!(t_sock->f_peername = 
	      tcmi_ctlfs_strfile_new(t_sock->d_id, TCMI_PERMS_FILE_R,
				     t_sock, tcmi_sock_getpeername, NULL,
				     KKC_SOCK_MAX_ADDR_LENGTH, "peername")))
		goto exit3;

	if (!(t_sock->f_archname = 
	      tcmi_ctlfs_strfile_new(t_sock->d_id, TCMI_PERMS_FILE_R,
				     t_sock, tcmi_sock_getarchname, NULL,
				     KKC_MAX_ARCH_LENGTH, "archname")))
		goto exit4;

	va_end(args);
	mdbg(INFO4, "Allocated new TCMI socket memory=%p", t_sock);
	return t_sock;

	/* error handling */
 exit4:
	tcmi_ctlfs_file_unregister(t_sock->f_peername);
	tcmi_ctlfs_entry_put(t_sock->f_peername);
 exit3:
	tcmi_ctlfs_file_unregister(t_sock->f_sockname);
	tcmi_ctlfs_entry_put(t_sock->f_sockname);
 exit2:
	tcmi_ctlfs_entry_put(t_sock->d_id);
 exit1:
	kkc_sock_put(t_sock->sock);
	kfree(t_sock);
	va_end(args);
 exit0:
	return NULL;
}


/** 
 * \<\<public\>\> Decrements the reference counter and if it reaches
 * 0, the instance. Unregistering the file is safe as the access is
 * internally serialized by TCMI ctlfs - the instance is removed from
 * the file and drop its reference counter.
 * 
 * @param *self - pointer to this listening instance
 */
void tcmi_sock_put(struct tcmi_sock *self)
{
	if (self && atomic_dec_and_test(&self->ref_count)) {
		mdbg(INFO4, "Destroying TCMI socket memory=%p", self);

		tcmi_ctlfs_file_unregister(self->f_archname);
		tcmi_ctlfs_entry_put(self->f_archname);

		tcmi_ctlfs_file_unregister(self->f_peername);
		tcmi_ctlfs_entry_put(self->f_peername);

		tcmi_ctlfs_file_unregister(self->f_sockname);
		tcmi_ctlfs_entry_put(self->f_sockname);

		tcmi_ctlfs_entry_put(self->d_id);
		kkc_sock_put(self->sock);
		kfree(self);
	}
}

/** @addtogroup tcmi_sock_class
 *
 * @{
 */

/** 
 * \<\<private\>\> Asks the KKC socket to supply its localname.
 * 
 * @param *object - pointer to this tcmi socket instance
 * @param *data - storage for the localname string
 * @return 0 upon success
 */
static int tcmi_sock_getsockname(void *object, void *data)
{
	struct tcmi_sock *self = TCMI_SOCK(object);

	kkc_sock_getsockname(self->sock, data, KKC_SOCK_MAX_ADDR_LENGTH);
	return 0;
}
 
/** 
 * \<\<private\>\> Asks the KKC socket to supply its peername.
 * 
 * @param *object - pointer to this tcmi socket instance
 * @param *data - storage for the peername string
 * @return 0 upon success
 */
static int tcmi_sock_getpeername(void *object, void *data)
{
	struct tcmi_sock *self = TCMI_SOCK(object);

	kkc_sock_getpeername(self->sock, data, KKC_SOCK_MAX_ADDR_LENGTH);
	return 0;
}

/** 
 * \<\<private\>\> Asks the KKC socket to supply its peername.
 * 
 * @param *object - pointer to this tcmi socket instance
 * @param *data - storage for the architecture string
 * @return 0 upon success
 */
static int tcmi_sock_getarchname(void *object, void *data)
{
	struct tcmi_sock *self = TCMI_SOCK(object);

	kkc_sock_getarchname(self->sock, data, KKC_MAX_ARCH_LENGTH);
	return 0;
}

/**
 * @}
 */


