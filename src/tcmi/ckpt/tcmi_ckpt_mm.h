/**
 * @file tcmi_ckpt_mm.h - a helper class that provides functionality to
 *                     store a single VM area (memory regions)
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_mm.h,v 1.3 2007/09/02 10:53:17 stavam2 Exp $
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
#ifndef _TCMI_CKPT_MM_H
#define _TCMI_CKPT_MM_H

#include <linux/mm.h>
#include <linux/mman.h>

#include <arch/types.h>
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

/** Compound structure describes a process memory descriptor.
 * Explanation if individual items is  e.g. in Understanding
 * the Linux Kernel(chapter 8)
 */
struct tcmi_ckpt_mm_hdr {
	/** code section, data section. */
	u_int64_t start_code, end_code, start_data, end_data;
	/** heap and stack. */
	u_int64_t start_brk, brk, start_stack;
	/** arguments and environment. */
	u_int64_t arg_start, arg_end, env_start, env_end;
	/** default access flags of the memory regions. */
	u_int64_t def_flags;
} __attribute__((__packed__));


/** 
 * Writes a current process memory descriptor into the checkpoint file. 
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_mm_write(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_mm_hdr mm_hdr;
        /* Now we dump the memory mapping */
        mm_hdr.start_code  = current->mm->start_code;
        mm_hdr.end_code    = current->mm->end_code;
        mm_hdr.start_data  = current->mm->start_data;
        mm_hdr.end_data    = current->mm->end_data;
        mm_hdr.start_brk   = current->mm->start_brk;
        mm_hdr.brk         = current->mm->brk;
        mm_hdr.start_stack = current->mm->start_stack;
        mm_hdr.arg_start   = current->mm->arg_start;
        mm_hdr.arg_end     = current->mm->arg_end;
        mm_hdr.env_start   = current->mm->env_start;
        mm_hdr.env_end     = current->mm->env_end;
        mm_hdr.def_flags   = current->mm->def_flags;



	/* write the header into the checkpoint */
	if (tcmi_ckpt_write(ckpt, &mm_hdr, sizeof(mm_hdr)) < 0) {
		mdbg(ERR3, "Error writing memory descriptor header");
		goto exit0;
	}
	mdbg(INFO4, "Written memory descriptor:");
	mdbg(INFO4, "START CODE :      %08lx", current->mm->start_code);
	mdbg(INFO4, "END CODE   :      %08lx", current->mm->end_code);
	mdbg(INFO4, "START DATA :      %08lx", current->mm->start_data);
	mdbg(INFO4, "END DATA   :      %08lx", current->mm->end_data);
	mdbg(INFO4, "START BRK  :      %08lx", current->mm->start_brk);
	mdbg(INFO4, "BRK        :      %08lx", current->mm->brk);
	mdbg(INFO4, "START STACK:      %08lx", current->mm->start_stack);
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
static inline int tcmi_ckpt_mm_read(struct tcmi_ckpt *ckpt)
{
	struct tcmi_ckpt_mm_hdr mm_hdr;
	/* write the header into the checkpoint */
	if (tcmi_ckpt_read(ckpt, &mm_hdr, sizeof(mm_hdr)) < 0) {
		mdbg(ERR3, "Error reading memory descriptor header");
		goto exit0;
	}
        current->mm->start_code  = CHECKED_UINT64_TO_ULONG(mm_hdr.start_code);
        current->mm->end_code    = CHECKED_UINT64_TO_ULONG(mm_hdr.end_code);
        current->mm->start_data  = CHECKED_UINT64_TO_ULONG(mm_hdr.start_data);
        current->mm->end_data    = CHECKED_UINT64_TO_ULONG(mm_hdr.end_data);
        current->mm->start_brk   = CHECKED_UINT64_TO_ULONG(mm_hdr.start_brk);
        current->mm->brk         = CHECKED_UINT64_TO_ULONG(mm_hdr.brk);
        current->mm->start_stack = CHECKED_UINT64_TO_ULONG(mm_hdr.start_stack);
        current->mm->arg_start   = CHECKED_UINT64_TO_ULONG(mm_hdr.arg_start);
        current->mm->arg_end     = CHECKED_UINT64_TO_ULONG(mm_hdr.arg_end);
        current->mm->env_start   = CHECKED_UINT64_TO_ULONG(mm_hdr.env_start);
        current->mm->env_end     = CHECKED_UINT64_TO_ULONG(mm_hdr.env_end);
        current->mm->def_flags   = CHECKED_UINT64_TO_ULONG(mm_hdr.def_flags);
	/* no regions left as they all have been flushed */
	current->mm->mmap = NULL;
	mdbg(INFO4, "Read memory descriptor:");
	mdbg(INFO4, "START CODE :      %08lx", current->mm->start_code);
	mdbg(INFO4, "END CODE   :      %08lx", current->mm->end_code);
	mdbg(INFO4, "START DATA :      %08lx", current->mm->start_data);
	mdbg(INFO4, "END DATA   :      %08lx", current->mm->end_data);
	mdbg(INFO4, "START BRK  :      %08lx", current->mm->start_brk);
	mdbg(INFO4, "BRK        :      %08lx", current->mm->brk);
	mdbg(INFO4, "START STACK:      %08lx", current->mm->start_stack);
	mdbg(INFO4, "ARG START:      %08lx", current->mm->arg_start);
	mdbg(INFO4, "ARG END:      %08lx", current->mm->arg_end);
	mdbg(INFO4, "ENV START:      %08lx", current->mm->env_start);
	mdbg(INFO4, "ENV END:      %08lx", current->mm->env_end);
	mdbg(INFO4, "DEF FLAGS:      %08lx", current->mm->def_flags);
	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_MM_PRIVATE


#endif /* TCMI_CKPT_MM_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_MM_H */

