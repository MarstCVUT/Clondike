/**
 * @file tcmi_penman.h - TCMI cluster core node manager - a class that
 *                       controls the cluster core node.
 *                      
 * 
 *
 *
 * Date: 04/06/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_penman.h,v 1.4 2007/11/05 19:38:28 stavam2 Exp $
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
#ifndef _TCMI_PENMAN_H
#define _TCMI_PENMAN_H





#include <linux/sched.h>

#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/ctlfs/tcmi_ctlfs_dir.h>
#include <tcmi/ctlfs/tcmi_ctlfs_file.h>

#include "tcmi_man.h"

/* TCMI PEN manager is controlled by compile time config option in Kbuild */
#ifdef CONFIG_TCMI_PEN
/** @defgroup tcmi_penman_class tcmi_penman class 
 * 
 * @ingroup tcmi_man_class
 *  
 * This \<\<singleton\>\> class implements the PEN TCMI manager
 * component. It is responsible for registering the cluster node to
 * CCN's specified by the user. Each migration manager communicates with one
 * CCN. User controls the process execution node by manipulating \link
 * tcmi_ctlfs.c TCMI control file system \endlink. The directory/file
 * layout can be divided into three groups of files/directories based
 * on their maintainers, which can be:
 *
 * - TCMI PEN manager
 * - TCMI PEN migration manager
 * - TCMI PEN preemptive migration component
 * - TCMI PEN non-preemtive migration component
 * - TCMI migrated process
 *
 * As we can see these are exact counterparts to the components on CCN.
 *
 \verbatim
  ------------ T C M I  P E N  m a n a g e r --------------
  pen/connect -> allows connecting to a CCN
  
      stop -> writing into this stops the whole framework, migrates all processes back 
             to their CCN's and terminates all migration managers.
 
  ----------- T C M I  P E N  m i g r a t i o n   m a n a g e r s --------------
      nodes/1/ -> each CCN where this PEN is registered has a separate directory, maintained
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
 
  ------------ T C M I  P E N  s h a d o w  p r o c e s s e s --------------
      mig/migproc/1300/ -> similar to /proc, but contains PID's of migrated processes
                           one directory per process
                  2001/
                     .
                     .    
                  6547/migman->../../nodes/7 -> a  symbolic link pointing to
                                                its current  CCN
		       remote-pid -> process PID at its original CCN
  ------------ T C M I  P E N  p p m  c o m p o n e n t --------------
                emigrate-ppm-p -> writing a PID + PEN id pair into this file starts process 
                            migration to the specified PEN.
                migrate-home -> allows migrating a specified process back to CCN
  ------------ T C M I  P E N  n p m  c o m p o n e n t (not implemented) --------------
                policy -> interface for the migration policy, the npm component
                          uses this interface to ask the migration policy for migration
                          decision.
 
 \endverbatim
 * 
 *
 * @todo InterPEN migration is not implemented.
 * @{
 */


struct npm_params;

/**
 * A compound structure that holds all current migration managers.
 * This is for internal use only.
 */
struct tcmi_penman {
	/** parent class instance. */
	struct tcmi_man super;

	/** TCMI ctlfs - connect control file */
	struct tcmi_ctlfs_entry *f_connect;

};
/** Casts to the PEN manager. */
#define TCMI_PENMAN(man) ((struct tcmi_penman *)man)

/** \<\<public\>\> Initializes the TCMI PEN manager. */
extern int tcmi_penman_init(struct tcmi_ctlfs_entry *root);

/** \<\<public\>\> Shuts down TCMI PEN manager. */
extern void tcmi_penman_shutdown(void);

/** \<\<public\>\> Getter of a pen man signelton instance. */
extern struct tcmi_penman* tcmi_penman_get_instance(void);

/** \<\<public\>\> Performs non-preemptive migration back to CCN  */
extern int tcmi_penman_migrateback_npm(struct tcmi_man *self, struct pt_regs* regs, struct tcmi_npm_params* npm_params);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_PENMAN_PRIVATE

/** TCMI PEN manager \<\<singleton\>\> instance. */
static struct tcmi_penman self;

/** Initializes TCMI ctlfs files. */
static int tcmi_penman_init_ctlfs_files(void);
/** Destroys all TCMI ctlfs files. */
static void tcmi_penman_stop_ctlfs_files(void);
/** Stops the TCMI PEN manager. */
static void tcmi_penman_stop(void);

/** Write method for the TCMI ctlfs - connects to a CCN. */
static int tcmi_penman_connect(void *obj, void *data);

/** CCN manager operations that support polymorphism */
static struct tcmi_man_ops penman_ops;

struct tcmi_penman* tcmi_penman_get_instance(void) {
	return &self;
};

#endif /* TCMI_PENMAN_PRIVATE */


/**
 * @}
 */
#else
/** 
 * Initializes the TCMI PEN manager, stub method when PEN not configured.
 *
 * @param *root - root directory of the filesystem, where all entries
 * are to be created
 * @return always 0;
 */
static inline int tcmi_penman_init(struct tcmi_ctlfs_entry *root)
{
	return 0;
}

/** Shuts down TCMI PEN manager, stub method when PEN not configured */
static inline void tcmi_penman_shutdown(void) 
{
}

#endif /* CONFIG_TCMI_PEN */

#endif /* _TCMI_PENMAN_H */

