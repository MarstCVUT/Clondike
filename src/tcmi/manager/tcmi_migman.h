/**
 * @file tcmi_migman.h - TCMI cluster core node manager - a class that
 *                       controls the cluster core node.
 *                      
 * 
 *
 *
 * Date: 04/19/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_migman.h,v 1.6 2009-01-20 14:23:03 andrep1 Exp $
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
#ifndef _TCMI_MIGMAN_H
#define _TCMI_MIGMAN_H

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>

#include <kkc/kkc.h>
#include <tcmi/comm/tcmi_messages_dsc.h>
#include <tcmi/comm/tcmi_comm.h>
#include <tcmi/ctlfs/tcmi_ctlfs.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>
#include <tcmi/lib/tcmi_sock.h>
#include <tcmi/lib/tcmi_slotvec.h>
#include <arch/arch_ids.h>

struct tcmi_task;

/** @defgroup tcmi_migman_class tcmi_migman class 
 *
 * @ingroup tcmi_managers_group
 *
 * TCMI migration manager controls migrating tasks for a particular
 * CCN-PEN pair. This is abstract class that gathers all common
 * functionality required for CCN and PEN resp. Node specific
 * implementation is carried out in \link tcmi_ccnmigman_class CCN
 * migration manager \endlink and \link tcmi_penmigman_class PEN
 * migration manager \endlink
 * 
 * Node specific migration manager supplies only methods to:
 * - release all its resources
 * - initialize and destroy TCMI ctlfs directories
 * - initialize and destroy TCMI ctlfs files
 * - process an incoming message from the message queue
 * - deliver a message to the message queue.
 * 
 * @{
 */

/** Describes the current state of the TCMI migration manager.
 * The states have following meaning:
 * - INIT - the system is being brought up
 * - CONNECTED - PEN has been succesfully authenticated and is now connected
 * - SHUTTING_DOWN - no more migration requests will be accepted, 
 *   migrated should be moved back or to other PEN 
 * - SHUT_DOWN - all connections are closed, component is ready
 *   for destruction.
 */
typedef enum {
	TCMI_MIGMAN_INIT,
	TCMI_MIGMAN_CONNECTED,
	TCMI_MIGMAN_SHUTTING_DOWN,
	TCMI_MIGMAN_SHUT_DOWN,
	/* number of states, not a special state */
	TCMI_MIGMAN_STATE_COUNT 
} tcmi_migman_state_t;


/**
 * A compound structure that holds all current shadow processes and
 * other migration related data.
 */
struct tcmi_migman {
	/** A slot node, used for insertion of the manager into a slot */
	tcmi_slot_node_t node;

	/** slot container for TCMI tasks(shadows on CCN /guests on
	 * PEN) that it controls.*/
	struct tcmi_slotvec *tasks;

	/** CCN ID - supplied by the instantiator(for CCN mig manager)
	 * or sent by the CCN(for PEN mig manager) */
	u_int32_t ccn_id;
	/** PEN ID - vica versa to how CCN ID is setup*/
	u_int32_t pen_id;
	/** Index of slot in associated manager, where this migman is placed */
	u_int slot_index;
	/** Architecture of corresponding migration manager peer (communicated on registration) */	
	enum arch_ids peer_arch_type;

	/** Instance reference counter. */
	atomic_t ref_count;

	/** Message queue for transaction-less messages. */
	struct tcmi_queue msg_queue;
	/** Message transactions. */
	struct tcmi_slotvec *transactions;

	/** Communication component used for message receiving and
	 * delivery. */
	struct tcmi_comm *comm;
	/** TCMI socket that represents an active connection to the PEN. */
	struct tcmi_sock *sock;

	/** Message processing thread */
	struct task_struct *msg_thread;

	/** Denotes the current state of the manager. */
	tcmi_migman_state_t state;

	/** TCMI ctlfs - directory where all migration control
	 * files/directories reside. */
	struct tcmi_ctlfs_entry *d_migman;
	/** TCMI ctlfs - directory for active connections */
	struct tcmi_ctlfs_entry *d_conns;
	/** TCMI ctlfs - reports current state of the manager. */
	struct tcmi_ctlfs_entry *f_state;

	/** TCMI ctlfs - reference to the migproc directory - needed
	 * when immigrating a process. */
	struct tcmi_ctlfs_entry *d_migproc;

	struct tcmi_ctlfs_entry *s_migman;

	/** Unique, randomly generated ID. */
	u_int32_t id;
	
	/** Operations specific to CCN/PEN migration manager resp. */
	struct tcmi_migman_ops *ops;
};

/** Migration manager operations that support polymorphism */
struct tcmi_migman_ops {
	/** Initializes TCMI ctlfs directories. */
	int (*init_ctlfs_dirs)(struct tcmi_migman*);
	/** Initializes TCMI ctlfs files. */
	int (*init_ctlfs_files)(struct tcmi_migman*);
	/** Destroys all TCMI ctlfs directories. */
	void (*stop_ctlfs_dirs)(struct tcmi_migman*);
	/** Destroys all TCMI ctlfs files. */
	void (*stop_ctlfs_files)(struct tcmi_migman*);
	/** Releases the instance specific resources. */
	void (*free)(struct tcmi_migman*);
	/** Processes a TCMI message m. */
	void (*process_msg)(struct tcmi_migman*, struct tcmi_msg*);
};

/** Message processing method type for the message processing thread. */
typedef void process_msg_t (struct tcmi_migman*, struct tcmi_msg*);
/** Casts to the migration manager. */
#define TCMI_MIGMAN(migman) ((struct tcmi_migman*)migman)


/** \<\<public\>\> TCMI migration manager initializer. 
 *
 * */
extern int tcmi_migman_init(struct tcmi_migman *self, struct kkc_sock *sock, 
			    u_int32_t ccn_id, u_int32_t pen_id, 
			    u_int slot_index, enum arch_ids peer_arch_type,
			    struct tcmi_ctlfs_entry *root,
			    struct tcmi_ctlfs_entry *migproc,
			    struct tcmi_migman_ops *ops,
			    const char namefmt[], va_list args);


/** 
 * \<\<public\>\> Instance accessor, increments the reference counter.
 *
 * @param *self - pointer to this instance
 * @return tcmi_migman instance
 */
static inline struct tcmi_migman* tcmi_migman_get(struct tcmi_migman *self)
{
	if (self) {
		atomic_inc(&self->ref_count);
	}
	return self;
}


/** Releases the instance. */
extern void tcmi_migman_put(struct tcmi_migman *self);

/** 
 * \<\<public\>\> Sets a new state of the migration manager.
 *
 * @param *self - pointer to this instance
 * @param state - new state to be set
 */
static inline void tcmi_migman_set_state(struct tcmi_migman *self, 
					 tcmi_migman_state_t state)
{
	self->state = state;
}

/** 
 * \<\<public\>\> Migration manager state accessor.
 *
 * @param *self - pointer to this instance
 * @return current state of the manager
 */
static inline tcmi_migman_state_t tcmi_migman_state(struct tcmi_migman *self)
{
	return self->state;
}

/** 
 * \<\<public\>\> Migration manager slot index getter
 *
 * @param *self - pointer to this instance
 * @return Index of slot in manager migmans vector
 */
static inline u_int tcmi_migman_slot_index(struct tcmi_migman *self)
{
	return self->slot_index;
}


/** 
 * \<\<public\>\> Migration manager root directory accessor.
 *
 * @param *self - pointer to this instance
 * @return current state of the manager
 */
static inline struct tcmi_ctlfs_entry* tcmi_migman_root(struct tcmi_migman *self)
{
	return self->d_migman;
}

/** 
 * \<\<public\>\> Transactions accessor.
 *
 * @param *self - pointer to this instance
 * @return transactions slot vector
 */
static inline struct tcmi_slotvec* tcmi_migman_transactions(struct tcmi_migman *self)
{
	return self->transactions;
}

/** 
 * \<\<public\>\> Message queue accesor.
 *
 * @param *self - pointer to this instance
 * @return message queue reference 
 */
static inline struct tcmi_queue* tcmi_migman_msg_queue(struct tcmi_migman *self)
{
	return &self->msg_queue;
}

/** 
 * \<\<public\>\> Communication socket accesor.  Reference counter is
 * not adjusted as the socket is part of the tcmi_sock instance. The
 * caller is expected to have a valid reference to the tcmi_migman
 * which in turn guarantees a reference to tcmi_sock.
 *
 * @param *self - pointer to this instance
 * @return pointer to KKC socket.
 */
static inline struct kkc_sock* tcmi_migman_sock(struct tcmi_migman *self)
{
	return tcmi_sock_kkc_sock_get(self->sock);
}


/** 
 * \<\<public\>\> Slot node accessor.
 *
 * @param *self - pointer to this instance
 * @return tcmi_slot_node
 */
static inline tcmi_slot_node_t* tcmi_migman_node(struct tcmi_migman *self)
{
	if (!self)
		return NULL;
	return &self->node;
}

/** 
 * \<\<public\>\> Migration manager ID accessor.
 *
 * @param *self - pointer to this instance
 * @return tcmi_slot_node
 */
static inline u_int32_t tcmi_migman_id(struct tcmi_migman *self)
{
	return self->id;
}

/** 
 * \<\<public\>\> Migproc directory accessor.
 *
 * @param *self - pointer to this instance
 * @return migproc directory where the migrated process publishes its
 * info
 */
static inline struct tcmi_ctlfs_entry* tcmi_migman_migproc_dir(struct tcmi_migman *self)
{
	return self->d_migproc;
}

/** 
 * \<\<public\>\> Getter of peer architecture type
 *
 * @param *self - pointer to this instance
 * @return architecture of peer computer
 */
static inline enum arch_ids tcmi_migman_peer_arch(struct tcmi_migman* self)
{
	return self->peer_arch_type;
}

/** \<\<public\>\> Registers tcmi task into the migration manager tasks slotvec */
extern int tcmi_migman_add_task(struct tcmi_migman *self, struct tcmi_task* task);
/** \<\<public\>\> Removes tcmi task from the migration manager tasks slotvec */
extern int tcmi_migman_remove_task(struct tcmi_migman *self, struct tcmi_task* task);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MIGMAN_PRIVATE

/** Frees all migration manager resources. */
static void tcmi_migman_free(struct tcmi_migman *self);

/** Initializes TCMI ctlfs directories. */
static int tcmi_migman_init_ctlfs_dirs(struct tcmi_migman *self,
				       struct tcmi_ctlfs_entry *root,
				       struct tcmi_ctlfs_entry *migproc,
				       const char namefmt[], va_list args);
/** Initializes TCMI ctlfs files. */
static int tcmi_migman_init_ctlfs_files(struct tcmi_migman *self);

/** Destroys all TCMI ctlfs directories. */
static void tcmi_migman_stop_ctlfs_dirs(struct tcmi_migman *self);
/** Destroys all TCMI ctlfs files. */
static void tcmi_migman_stop_ctlfs_files(struct tcmi_migman *self);

/** Read method for the TCMI ctlfs - reports migration manager state. */
static int tcmi_migman_show_state(void *obj, void *data);

/** Starts the TCMI migration manager message processing thread. */
static int tcmi_migman_start_thread(struct tcmi_migman *self);
/** Stops the TCMI migration manager message processing thread. */
static void tcmi_migman_stop_thread(struct tcmi_migman *self);
/** Delivers a TCMI message to the mig. manager or to a task */
static int tcmi_migman_deliver_msg(void *obj, struct tcmi_msg *m);
/** TCMI manager message processing thread. */
static int tcmi_migman_msg_thread(void *data);

/** */
int tcmi_migman_create_symlink(struct tcmi_migman *self, struct tcmi_ctlfs_entry * root, struct kkc_sock *sock);

/** String descriptor for the states of migration manager - forware declaration. */
static char *tcmi_migman_states[];

#endif /* TCMI_MIGMAN_PRIVATE */

/**
 * @}
 */


#endif /* _TCMI_MIGMAN_H */

