/**
 * @file tcmi_ccnman.h - TCMI cluster core node manager - a class that
 *                       controls the cluster core node.
 *                      
 * 
 *
 *
 * Date: 04/06/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ccnman.h,v 1.4 2007/09/15 14:46:09 stavam2 Exp $
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
#ifndef _TCMI_CCNMAN_H
#define _TCMI_CCNMAN_H

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>

#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>
#include <tcmi/migration/fs/fs_mount_params.h>

#include "tcmi_man.h"

/* TCMI CCN manager is controlled by compile time config option in Kbuild */
#ifdef CONFIG_TCMI_CCN
/** @defgroup tcmi_ccnman_class tcmi_ccnman class 
 *
 * @ingroup tcmi_man_class
 *
 * This \<\<singleton\>\> class implements the CCN TCMI manager
 * component. It is responsible for registration of new PEN's.  Each
 * migration manager is associated with one registered PEN. The user
 * controls the cluster core node by manipulating \link tcmi_ctlfs.c
 * TCMI control file system \endlink. The directory/file layout can be
 * divided into three groups of files/directories based on their
 * maintainers, which can be:
 *
 * - TCMI CCN manager
 * - TCMI CCN migration manager
 * - TCMI CCN preemptive migration component
 * - TCMI CCN non-preemtive migration component
 * - TCMI shadow process
 *
 * 
 \verbatim
  ------------ T C M I  m a n a g e r --------------
  ccn/listen -> allows adding new interfaces where the CCN manager 
               will be listening on.
      listening-on/1/  -> each listening interface has one associated directory
                   2/
                   .
                   .
                   5/iface -> file that contains the interface address

      stop-listen-all -> writing into it terminates listening on all interfaces
 
      stop-listen-one -> writing into it terminates specified listening

      stop -> writing into this stops all listenings, migrates all processes back 
             and terminates all migration managers.
 
  ----------- T C M I  m i g r a t i o n   m a n a g e r s --------------
      nodes/1/ -> each registered PEN has a separate directory, maintained
                  by its migration manager that is in charge of it.
            2/
            .
            .
            9/connections/ -> all connections of a migration manager
	                  ctrlconn/ -> control connection
                                   arch -> contains architecture of the connection
				   localname -> local socket name (address)
				   peername -> remote socket name (address)
	      state - current state of the migration manager
              load -> average load of the PEN, used for migration decisions
 
  ------------ T C M I  s h a d o w  p r o c e s s e s --------------
      mig/migproc/1300/ -> similar to /proc, but contains PID's of migrated processes
                           one directory per process
                  2001/
                     .
                     .    
                  6547/migman->../../nodes/7 -> a  symbolic link pointing to
                                                its current  PEN 
		       remote-pid -> process PID at its current PEN
  ------------ T C M I  p p m  c o m p o n e n t --------------
                emigrate-ppm-p -> writing a PID + PEN id pair into this file starts process 
                            migration to the specified PEN.
                migrate-home -> allows migrating a specified process back CCN
  ------------ T C M I  n p m  c o m p o n e n t (not implemented) --------------
                policy -> interface for the migration policy, the npm component
                          uses this interface to ask the migration policy for migration
                          decision.
 
 \endverbatim
 *
 * As has been mentioned, the TCMI CCN manager is responsible for PEN registration.
 * Handling incoming connections and instantiation of new migration managers requires
 * a separate execution thread. 
 *
 * Generally, we assume a limited number of interfaces that the CCN
 * will be listening on. The core datastructure for keeping track of
 * listenings is \link tcmi_slotvec_class a slot vector \endlink.
 *
 * 
 *
 * @{
 */

/**
 * A compound structure that holds all current listenings.
 * This is for internal use only.
 */
struct tcmi_ccnman {
	/** parent class instance. */
	struct tcmi_man super;

	/** slot container for listings, 1 listening per slot */
	struct tcmi_slotvec *listenings;

	/** TCMI ctlfs - listen control file */
	struct tcmi_ctlfs_entry *f_listen;
	/** TCMI ctlfs - directory for active listenings */
	struct tcmi_ctlfs_entry *d_listening_on;
	/** TCMI ctlfs - stop - listen - all control file */
	struct tcmi_ctlfs_entry *f_stop_listen_all;
	/** TCMI ctlfs - stop - listen - one control file */
	struct tcmi_ctlfs_entry *f_stop_listen_one;

	/** TCMI ctlfs - directory for moutner options */
	struct tcmi_ctlfs_entry *d_mounter;
	/** TCMI ctlfs - remote mount method. */
	struct tcmi_ctlfs_entry *f_mount;
	/** TCMI ctlfs - remote mount device. */
	struct tcmi_ctlfs_entry *f_mount_device;
	/** TCMI ctlfs - remote mount options. */
	struct tcmi_ctlfs_entry *f_mount_options;
	/** Remote moutn params */
	struct fs_mount_params mount_params;

	/** Listening thread reference */
	struct task_struct *thread;

	/** list of sleeper elements for each socket */
	struct list_head sleepers;
	/** protects the list from concurrent access */
	spinlock_t sleepers_lock;
};
/** Casts to the CCN manager. */
#define TCMI_CCNMAN(man) ((struct tcmi_ccnman *)man)

/** \<\<public\>\> Initializes the TCMI CCN manager. */
extern int tcmi_ccnman_init(struct tcmi_ctlfs_entry *root);

/** \<\<public\>\> Shuts down TCMI CCN manager. */
extern void tcmi_ccnman_shutdown(void);

/** \<\<public\>\> Getter of mount params structure. */
extern struct fs_mount_params* tcmi_ccnman_get_mount_params(void);

extern struct tcmi_ccnman* tcmi_ccnman_get_instance(void);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CCNMAN_PRIVATE

/** TCMI CCN manager \<\<singleton\>\> instance. */
static struct tcmi_ccnman self;

/** Initializes TCMI ctlfs directories. */
static int tcmi_ccnman_init_ctlfs_dirs(void);
/** Initializes TCMI ctlfs files. */
static int tcmi_ccnman_init_ctlfs_files(void);
/** Destroys all TCMI ctlfs directories. */
static void tcmi_ccnman_stop_ctlfs_dirs(void);
/** Destroys all TCMI ctlfs files. */
static void tcmi_ccnman_stop_ctlfs_files(void);
/** Stops the TCMI CCN manager. */
static void tcmi_ccnman_stop(void);


/** Write method for the TCMI ctlfs - adds new listening. */
static int tcmi_ccnman_listen(void *obj, void *data);

/** Write method for the TCMI ctlfs - stops all listenings. */
static int tcmi_ccnman_stop_listen_all(void *obj, void *data);

/** Write method for the TCMI ctlfs - stops a selected listening. */
static int tcmi_ccnman_stop_listen_one(void *obj, void *data);


/** Write method for the TCMI ctlfs - set mount types. */
static int tcmi_man_set_mount(void *obj, void *data);
/** Write method for the TCMI ctlfs - set mount device. */
static int tcmi_man_set_mount_device(void *obj, void *data);
/** Write method for the TCMI ctlfs - set mount options. */
static int tcmi_man_set_mount_options(void *obj, void *data);

/** Starts the TCMI CCN manager listening thread. */
static int tcmi_ccnman_start_thread(void);

/** Stops the TCMI CCN manager listening thread. */
static void tcmi_ccnman_stop_thread(void);

/** TCMI CCN manager listening thread. */
static int tcmi_ccnman_thread(void *data);

/** Processes a selected socket, checking for incoming connection. */
static void tcmi_ccnman_process_sock(struct kkc_sock *);

/** Adds a sleeper to the list. */
static inline void tcmi_ccnman_add_sleeper(struct kkc_sock_sleeper *);


/** Removes sleepers that sleep on the socket. */ 
static inline void tcmi_ccnman_remove_sleepers(struct tcmi_sock *);


/** CCN manager operations that support polymorphism */
static struct tcmi_man_ops ccnman_ops;

struct tcmi_ccnman* tcmi_ccnman_get_instance(void) {
	return &self;
};

#endif /* TCMI_CCNMAN_PRIVATE */


/**
 * @}
 */

#else
/** 
 * Initializes the TCMI PEN manager, stub method when CCN not configured 
 *
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @return always 0;
 */
static inline int tcmi_ccnman_init(struct tcmi_ctlfs_entry *root)
{
	return 0;
}

/** Shuts down TCMI CCN manager, stub method when CCN not configured */
static inline void tcmi_ccnman_shutdown(void) 
{
}

#endif /* CONFIG_TCMI_CCN */

#endif /* _TCMI_CCNMAN_H */

