/**
 * @file tcmi_ckptcom.h - Process checkpointing component.
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckptcom.h,v 1.3 2007/09/02 21:49:42 stavam2 Exp $
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
#ifndef _TCMI_CKPTCOM_H
#define _TCMI_CKPTCOM_H

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/binfmts.h>

struct tcmi_npm_params;

/** @defgroup tcmi_ckptcom_class checkpointing component
 *
 * @ingroup tcmi_module_class
 *
 * \<\<Singleton\>\> class that creates a checkpoint of a process.
 * It uses the checkpoint class as a temporary object to store
 * the state of the process.
 *
 * It provides a simple interface that that allows:
 * - creating a checkpoint into a specified file
 * - restoring the checkpoint via a new bin_fmt handler that it
 * registers in the kernel.
 *
 * While creating a checkpoint there are two types of checkpoints
 * supported - heavy and light. See tcmi_ckptcom_checkpoint() for
 * explanation.
 *
 * @{
 */

/** \<\<public\>\> Creates a preemptive process checkpoint. */
extern int tcmi_ckptcom_checkpoint_ppm(struct file *file, struct pt_regs *regs, int heavy);
/** \<\<public\>\> Creates a non-preemptive process checkpoint. */
extern int tcmi_ckptcom_checkpoint_npm(struct file *file, struct pt_regs *regs, struct tcmi_npm_params* params);
/** \<\<public\>\> Restarts a process from a checkpoint (new binfmt handler). */
extern int tcmi_ckptcom_restart(struct linux_binprm *bprm, struct pt_regs *regs);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPTCOM_PRIVATE
/** New checkpoint binary format for the kernel. */
static struct linux_binfmt tcmi_ckptcom_format;

#endif /* TCMI_CKPTCOM_PRIVATE */


/**
 * @}
 */

#endif /* _TCMI_CKPTCOM_H */

