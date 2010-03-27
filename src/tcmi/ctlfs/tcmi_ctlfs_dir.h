/**
 * @file tcmi_ctlfs_dir.h - Declaration of a class that represents directory
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
 * $Id: tcmi_ctlfs_dir.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
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
#ifndef TCMI_CTLFS_DIR_H
#define TCMI_CTLFS_DIR_H

#include "tcmi_ctlfs_entry.h"

/** @defgroup tcmi_ctlfs_dir_class tcmi_ctlfs_dir class 
 * 
 * @ingroup tcmi_ctlfs_entry_class
 * 
 * This class represents a directory in the tcmi control file system.
 * It provides a functionality to instantiate a root directory and
 * regular directories. The root directory is special as it doesn't
 * have a parent entry.
 *
 *@{
 */

/**
 * A directory compound structure, extends the parent entry class.
 */
struct tcmi_ctlfs_dir {
	/** parent class instance */
	struct tcmi_ctlfs_entry super;
};


/** \<\<public\>\> Creates a new directory instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_dir_new(struct tcmi_ctlfs_entry *parent,
						   mode_t mode, const char namefmt[], ...);

/** \<\<public\>\> Creates a new directory instance - va_list version. */
struct tcmi_ctlfs_entry* tcmi_ctlfs_dir_vnew(struct tcmi_ctlfs_entry *parent,
					     mode_t mode, const char namefmt[], 
					     va_list args);

/** \<\<public\>\> Creates a root directory instance. */
extern struct tcmi_ctlfs_entry* tcmi_ctlfs_rootdir_new(struct super_block *sb);

/** Casts an entry to directory */
#define TCMI_CTLFS_DIR(e) ((struct tcmi_ctlfs_dir *)e)

/**
 * @}
 */
	

#endif /* TCMI_CTLFS_DIR_H */
