/**
 * @file tcmi_ckpt_sig.h - a class that provides functionality to
 *                     store a signal settings
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_ckpt_sig.h,v 1.3 2007/09/03 01:17:58 malatp1 Exp $
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
#ifndef _TCMI_CKPT_SIG_H
#define _TCMI_CKPT_SIG_H

#include <linux/sched.h>
#include <linux/signal.h>

#include "tcmi_ckpt.h"

/** @defgroup tcmi_ckpt_mm_class tcmi_ckpt_mm class 
 *
 * @ingroup tcmi_ckpt_class
 *
 * This is a \<\<singleton\>\> that takes care of (re)storing a
 * process memory descriptor.
 *
 *
 * 
 * @{
 */

struct tcmi_sigset {
	u_int64_t sigs;
} __attribute__((__packed__));


struct tcmi_k_sigaction {
	u_int64_t sa_handler;
	u_int64_t sa_restorer;
	u_int64_t sa_flags;	
	struct tcmi_sigset sa_mask;
} __attribute__((__packed__));

/** Compound structure describes a process memory descriptor.
 * Explanation if individual items is  e.g. in Understanding
 * the Linux Kernel(chapter 8)
 */
struct tcmi_ckpt_sig_hdr {
	/** Ponter to alternative signal handler stack */
	u_int64_t sas_ss_sp;
	/** Alternative signal handler stack size */
	u_int64_t sas_ss_size;
	/** Blocked signals */
	struct tcmi_sigset blocked;
	/** Blocked signals */
	struct tcmi_sigset real_blocked;
	/** Signal actions */
	struct tcmi_k_sigaction action[_NSIG];
} __attribute__((__packed__));


/** 
 * Writes a current process memory descriptor into the checkpoint file. 
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_sig_write(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_sig_hdr sig_hdr;
	int i;

        /* Now we dump the memory mapping */
        sig_hdr.sas_ss_sp   = current->sas_ss_sp;
        sig_hdr.sas_ss_size = current->sas_ss_size;
	//for(i = 0; i < _NSIG_WORDS; i++){ // Only few bytes
	        sig_hdr.blocked.sigs       = *(u_int64_t*)(void*)&current->blocked;
	        sig_hdr.real_blocked.sigs  = *(u_int64_t*)(void*)&current->real_blocked;
	//}

	// Add pending signals
	for(i = 0; i < _NSIG; i++){
		sig_hdr.action[i].sa_handler = (u_int64_t)(long)current->sighand->action[i].sa.sa_handler;
		sig_hdr.action[i].sa_restorer = (u_int64_t)(long)current->sighand->action[i].sa.sa_restorer;
		sig_hdr.action[i].sa_flags = current->sighand->action[i].sa.sa_flags;
		sig_hdr.action[i].sa_mask.sigs = *(u_int64_t*)(void*)&current->sighand->action[i].sa.sa_mask;
	}
	

	/* write the header into the checkpoint */
	if (tcmi_ckpt_write(ckpt, &sig_hdr, sizeof(sig_hdr)) < 0) {
		mdbg(ERR3, "Error writing signal header");
		goto exit0;
	}
	mdbg(INFO4, "Written signal header");
	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;

}

/** 
 * Reads a memory area from the checkpoint file and sets the those in
 * the memory descriptor. This also assumes that the old execution
 * context has already been flushed by the caller.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_sig_read(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_sig_hdr sig_hdr;
	int i;

	/* write the header into the checkpoint */
	if (tcmi_ckpt_read(ckpt, &sig_hdr, sizeof(sig_hdr)) < 0) {
		mdbg(ERR3, "Error reading signals");
		goto exit0;
	}
        current->sas_ss_sp   = sig_hdr.sas_ss_sp;
        current->sas_ss_size = sig_hdr.sas_ss_size;
	//for(i = 0; i < _NSIG_WORDS; i++){ // Only few bytes
		current->blocked      = *(sigset_t*)&sig_hdr.blocked.sigs;
		current->real_blocked = *(sigset_t*)&sig_hdr.real_blocked.sigs;
	//}
	// Signal actions
	for(i = 0; i < _NSIG; i++){
		current->sighand->action[i].sa.sa_handler = (__sighandler_t)(long)sig_hdr.action[i].sa_handler;
		current->sighand->action[i].sa.sa_restorer = (__sigrestore_t)(long)sig_hdr.action[i].sa_restorer;
		current->sighand->action[i].sa.sa_flags = sig_hdr.action[i].sa_flags;
		current->sighand->action[i].sa.sa_mask = *(sigset_t*)&sig_hdr.action[i].sa_mask.sigs;
	}
	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_SIG_PRIVATE


#endif /* TCMI_CKPT_SIG_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_SIG_H */

