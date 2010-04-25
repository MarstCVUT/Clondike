/**
 * @file tcmi_man.h - TCMI cluster node manager - a template class
 *                    that is further implemented by CCN/PEN
 *                      
 * 
 *
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_man.h,v 1.6 2007/10/20 14:24:20 stavam2 Exp $
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
#ifndef _TCMI_MAN_H
#define _TCMI_MAN_H

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/types.h>

#include <tcmi/manager/tcmi_migman.h>
#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>

struct tcmi_npm_params;

/** @defgroup tcmi_man_class tcmi_man class 
 *
 * @ingroup tcmi_managers_group
 * 
 * This template \<\<singleton\>\> class implements a generic cluster
 * node manager - TCMI manager. It provides generic functionality to control
 * migration managers. 
 *
 * Each migration manager is associated with one registered PEN. The
 * user controls the cluster node(PEN,CCN or both) by manipulating
 * \link tcmi_ctlfs.c TCMI control file system \endlink. The
 * directory/file layout can be divided into three groups of
 * files/directories based on their maintainers, which can be:
 *
 * - TCMI manager
 * - TCMI migration manager
 * - TCMI preemptive migration component
 * - TCMI non-preemtive migration component
 * - TCMI tasks
 *
 *
 * Description of a particular layout is detailed at \link
 * tcmi_ccnman_class TCMI CCN manager \endlink and \link
 * tcmi_penman_class TCMI PEN manager \endlink
 *
 *
 * There is an upper bound on the number of registered nodes. This is
 * a very common approach in UNIX design as it prevents problems with
 * overloading the CCN. This parameter is currently set at compile
 * time.  but might be made configurable via ctlfs from user space.
 *
 * The core datastructure for keeping track of migration
 * managers(=registered cluster nodes) is \link tcmi_slotvec_class a
 * slot vector \endlink.
 *
 * The TCMI manager carries a unique identifier that is generated
 * randomly upon initialization. This ID then identifies a node in
 * cluster.
 *
 * @{
 */

/**
 * A compound structure that holds all current listenings and migration managers.
 * This is for internal use only.
 */
struct tcmi_man {
	/** indicates the manager is ready - used for proper shutdown */
	atomic_t ready;

	/** slot container for migration managers, 1 manager per slot */
	struct tcmi_slotvec *mig_mans;


	/** root directory of CCN or PEN where all control
	 * files/directories reside */
	struct tcmi_ctlfs_entry *d_man;
	/** directory where all registered nodes (CCN's or PEN's) reside */
	struct tcmi_ctlfs_entry *d_nodes;

	/** TCMI ctlfs - migration directory */
	struct tcmi_ctlfs_entry *d_mig;

	/** TCMI ctlfs - migproc directory for migrated processes */
	struct tcmi_ctlfs_entry *d_migproc;

	/** TCMI ctlfs - stop control file */
	struct tcmi_ctlfs_entry *f_stop;

	/** TCMI ctlfs - migration control file (PPM physical ckpt.) */
	struct tcmi_ctlfs_entry *f_emig_ppm_p;

	/** TCMI ctlfs - migration control file (PPM physical ckpt.) */
	struct tcmi_ctlfs_entry *f_mig_home_ppm_p;

	/** Unique manager ID. */
	u_int32_t id;

	/** Operations specific to a CCN/PEN manager resp. */
	struct tcmi_man_ops *ops;
};
/** Migration manager operations that support polymorphism */
struct tcmi_man_ops {
	/** Initializes TCMI ctlfs directories. */
	int (*init_ctlfs_dirs)(void);
	/** Initializes TCMI ctlfs files. */
	int (*init_ctlfs_files)(void);
	/** Destroys all TCMI ctlfs directories. */
	void (*stop_ctlfs_dirs)(void);
	/** Destroys all TCMI ctlfs files. */
	void (*stop_ctlfs_files)(void);
	/** Preemptive emigration method. */
	int (*emigrate_ppm_p)(pid_t, struct tcmi_migman*);
	/** Non-preemptive emigration method. */
	int (*emigrate_npm)(pid_t, struct tcmi_migman*, struct pt_regs* regs, struct tcmi_npm_params*);
	/** Migrate home method. */
	int (*migrate_home_ppm_p)(pid_t);	
	/** Transforms forked process to a same type of task as parent and assigns it with a same manager. */
	int (*fork)(struct task_struct* parent, struct task_struct* child, struct tcmi_migman*);

	/** Instance specific stop method. */
	void (*stop)(void);
};

/** Casts to the TCMI manager. */
#define TCMI_MAN(man) ((struct tcmi_man *)man)


/** \<\<public\>\> Initializes the TCMI manager */
extern int tcmi_man_init(struct tcmi_man *self,  
			 struct tcmi_ctlfs_entry *root,
			 struct tcmi_man_ops *ops,
			 const char *man_dirname);

/** \<\<public\>\> Shuts down TCMI manager */
extern void tcmi_man_shutdown(struct tcmi_man *self);

/** \<\<public\>\> Send generic user message */
extern int tcmi_man_send_generic_user_message(struct tcmi_man *self, int target_slot_index, int user_data_size, char* user_data);

/**
 * \<\<public\>\> Reserves an empty slot for a new migration manager.
 *
 * @param *self - a particular manager singleton instance
 * @return an empty slot(if available) or NULL
 */
static inline struct tcmi_slot* tcmi_man_reserve_migmanslot(struct tcmi_man *self)
{
	return tcmi_slotvec_reserve_empty(self->mig_mans);
}


/**
 * \<\<public\>\> Releases a migration slot that has been previously
 * reserved.  It is needed when new migration manager fails and won't
 * inserted into the slot. The user that returns the slot back to the
 * vector.
 *
 * @param *self - a particular manager singleton instance
 * @param *slot thatis to be release back to the vector
 */
static inline void tcmi_man_release_migmanslot(struct tcmi_man *self, 
					       struct tcmi_slot *slot)
{
	tcmi_slot_move_unused(slot); 
}

/**
 * \<\<public\>\> Accessor for a main(root) directory of a particular
 * TCMI manager.
 *
 * @param *self - a particular manager singleton instance
 * @return main manager directory
 */
static inline struct tcmi_ctlfs_entry* tcmi_man_root(struct tcmi_man *self)
{
	return self->d_man;
}

/**
 * \<\<public\>\> Accessor for a cluster nodes directory of a
 * particular TCMI manager.
 *
 * @param *self - a particular manager singleton instance
 * @return nodesdirectory
 */
static inline struct tcmi_ctlfs_entry* tcmi_man_nodes_dir(struct tcmi_man *self)
{
	return self->d_nodes;
}

/**
 * \<\<public\>\> Accessor for migproc directory.
 *
 * @param *self - a particular manager singleton instance
 * @return migproc directory
 */
static inline struct tcmi_ctlfs_entry* tcmi_man_migproc_dir(struct tcmi_man *self)
{
	return self->d_migproc;
}

/**
 * \<\<public\>\> TCMI manager ID accessor.
 *
 * @param *self - a particular manager singleton instance
 * @return manager ID
 */
static inline u_int32_t tcmi_man_id(struct tcmi_man *self)
{
	return self->id;
}

/** \<\<public\>\> Method that performs non-preemtive migration  */
extern int tcmi_man_emig_npm(struct tcmi_man *self, pid_t pid, u_int32_t migman_id, struct pt_regs* regs, struct tcmi_npm_params* npm_params);

/** \<\<public\>\> Method that performs task fork */
extern int tcmi_man_fork(struct tcmi_man *self, struct task_struct* parent, struct task_struct* child);


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_MAN_PRIVATE

/** Initializes TCMI ctlfs directories. */
static int tcmi_man_init_ctlfs_dirs(struct tcmi_man *self,
				    struct tcmi_ctlfs_entry *root,
				    const char *man_dirname);
/** Initializes TCMI ctlfs files. */
static int tcmi_man_init_ctlfs_files(struct tcmi_man *self);

/** Destroys all TCMI ctlfs directories. */
static void tcmi_man_stop_ctlfs_dirs(struct tcmi_man *self);
/** Destroys all TCMI ctlfs files. */
static void tcmi_man_stop_ctlfs_files(struct tcmi_man *self);

/** Initializes the migration components. */
static int tcmi_man_init_migration(struct tcmi_man *self);
/** Stops the migration components. */
static void tcmi_man_stop_migration(struct tcmi_man *self);

/** TCMI ctlfs write method - stops the manager. */
static int tcmi_man_stop(void *obj, void *data);

/** TCMI ctlfs write method - emigration PPM P */
static int tcmi_man_emig_ppm_p(void *obj, void *data);
/** TCMI ctlfs write method - migration home */
static int tcmi_man_mig_home_ppm_p(void *obj, void *data);

#endif /* TCMI_MAN_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_MAN_H */

