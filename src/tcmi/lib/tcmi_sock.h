/**
 * @file tcmi_sock.h - A wrapper class for KKC listenings.
 *                       
 *                      
 * 
 *
 *
 * Date: 04/17/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_sock.h,v 1.3 2007/10/07 15:54:00 stavam2 Exp $
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
#ifndef _TCMI_SOCK_H
#define _TCMI_SOCK_H

#include <asm/atomic.h>

#include "tcmi_slot.h"
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>

#include <kkc/kkc.h>

/** @defgroup tcmi_sock_class tcmi_sock class 
 *
 * @ingroup tcmi_lib_group
 * 
 * This class is a wrapper for KKC socket. It provides a TCMI ctlfs
 * user interface, that allows the user to read information about
 * the socket - local name, peername and architecture.
 *
 * Sockets are meant to be stored in \link tcmi_slotvec_class a
 * slot vector \endlink. For that reason they provide a slot node,
 * so that they can be linked into the slotvector.
 *
 * There is no need for additional lock since access to the
 * data structures from user space is serialized via the TCMI ctlfs
 * files. Other kernel threads accessing the socket don't manipulate
 * its internal datastructures, except for the last thread that
 * destroys it. This protection is implemented by standard reference
 * counting.
 *
 * @{
 */

/**
 * A compound structure that holds the actual KKC listening.
 */
struct tcmi_sock {
	/** A slot node, used for insertion the listening into a slot */
	tcmi_slot_node_t node;

	/** Instance reference counter. */
	atomic_t ref_count;

	/** TCMI ctlfs directory where there are the control files of
	 * the listening. The name is based on slot index */
	struct tcmi_ctlfs_entry *d_id;
	/** TCMI ctlfs - shows the local socket address. */
	struct tcmi_ctlfs_entry *f_sockname;
	/** TCMI ctlfs - shows the remote socketa addres - meaningful
	 * for connected sockets only. */
	struct tcmi_ctlfs_entry *f_peername;
	/** TCMI ctlfs - shows the architecture name. */
	struct tcmi_ctlfs_entry *f_archname;

	/** KKC socket contained in the instance. */
	struct kkc_sock *sock;
};

/** \<\<public\>\> TCMI socket constructor. */
extern struct tcmi_sock* tcmi_sock_new(struct tcmi_ctlfs_entry *root,
				       struct kkc_sock *sock,
				       const char namefmt[], ...);


/** 
 * \<\<public\>\> KKC socket accessor.  Reference counter of the KKC
 * socket is not adjusted as the socket is part of the tcmi_sock
 * instance. The caller is expected to have a valid reference to the
 * tcmi_sock instance.
 *
 * @param *self - pointer to this instance
 * @return tcmi_sock instance
 */
static inline struct kkc_sock* tcmi_sock_kkc_sock_get(struct tcmi_sock *self)
{
	if (!self)
		return NULL;
	return self->sock;
}

/** 
 * \<\<public\>\> Instance accessor. increments the reference counter.
 * The user is now guaranteed, that the instance stays in memory while
 * he is using it.
 *
 * @param *self - pointer to this instance
 * @return tcmi_sock instance
 */
static inline struct tcmi_sock* tcmi_sock_get(struct tcmi_sock *self)
{
	if (self) {
/*		mdbg(CRIT4, "Incrementing ref. count(%d) of local:'%s', remote:'%s' (%p)", 
		     atomic_read(&self->ref_count), 
		     kkc_sock_getsockname2(tcmi_sock_kkc_sock_get(self)), 
		     kkc_sock_getpeername2(tcmi_sock_kkc_sock_get(self)), self);
*/
		atomic_inc(&self->ref_count);
	}
	return self;
}


/** 
 * \<\<public\>\> Sock node accessor.
 *
 * @param *self - pointer to this instance
 * @return tcmi_slot_node
 */
static inline tcmi_slot_node_t* tcmi_sock_node(struct tcmi_sock *self)
{
	if (!self)
		return NULL;
	return &self->node;
}

/** \<\<public\>\> Releases the instance. */
extern void tcmi_sock_put(struct tcmi_sock *self);

/** Casts to a tcmi_sock instance */
#define TCMI_SOCK(l) ((struct tcmi_sock *)l)

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_SOCK_PRIVATE

/** Read method for the TCMI ctlfs - accessor for socketname string. */
static int tcmi_sock_getsockname(void *object, void *data);
/** Read method for the TCMI ctlfs - accessor for peername name string. */
static int tcmi_sock_getpeername(void *object, void *data);
/** Read method for the TCMI ctlfs - accessor for architecture name string. */
static int tcmi_sock_getarchname(void *object, void *data);

#endif /* TCMI_SOCK_PRIVATE */


/**
 * @}
 */



#endif /* _TCMI_SOCK_H */
