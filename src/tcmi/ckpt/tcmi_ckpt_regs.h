/**
 * @file tcmi_ckpt_regs.h - a helper class that provides functionality to
 *                     store process registers
 *                      
 * 
 *
 *
 * Date: 05/02/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_regs.h,v 1.5 2007/09/15 14:46:09 stavam2 Exp $
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
#ifndef _TCMI_CKPT_REGS_H
#define _TCMI_CKPT_REGS_H

#include <asm/uaccess.h>
#include <arch/current/regs.h>

#include "tcmi_ckpt.h"

/** 
 * Writes a current process registers into the checkpoint file.
 * Adjusts the EIP value if the process has aborted a system call.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *regs - registers of the current process
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_regs_write(struct tcmi_ckpt *ckpt, struct pt_regs *regs)
{	
	struct tcmi_regs tcmi_regs;
	int32_t regs_length;

	mdbg(INFO4, "Getting registers: %p", regs);

	/* copy the process registers into the header */
	get_registers(regs, &tcmi_regs);

	regs_length = sizeof(tcmi_regs);
	/* write length of the registers structure to ckpt (every architecture can have different length) */
	if (tcmi_ckpt_write(ckpt, &regs_length, sizeof(regs_length)) < 0) {
		mdbg(ERR3, "Error writing registers length");
		goto exit0;
	}

	/* write the registers into the checkpoint */
	if (tcmi_ckpt_write(ckpt, &tcmi_regs, sizeof(tcmi_regs)) < 0) {
		mdbg(ERR3, "Error writing registers");
		goto exit0;
	}

	mdbg(INFO4, "Written process registers:");
	debug_registers(regs);

	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;

}

/** 
 * Reads a process registers from the checkpoint file and sets
 * them for the current process.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *regs - registers of the current process
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_regs_read(struct tcmi_ckpt *ckpt, struct pt_regs *regs)
{
	void* platform_regs = NULL;
	int32_t regs_length;

	if (tcmi_ckpt_read(ckpt, &regs_length, sizeof(regs_length)) < 0) {
		mdbg(ERR3, "Error reading process register length");
		goto exit0;
	}

	platform_regs = kmalloc(regs_length, GFP_KERNEL);
	if ( !platform_regs ) {
		mdbg(ERR3, "Not enough memory to load registers");
		goto exit0;
	}

	/* Read the regisiters */
	if (tcmi_ckpt_read(ckpt, platform_regs, regs_length) < 0) {
		mdbg(ERR3, "Error reading process register descriptor header");
		goto exit1;
	}

	load_from_platform_registers(platform_regs, regs, ckpt->hdr.checkpoint_arch, ckpt->hdr.is_32bit_application);
	platform_start_thread(regs, instruction_pointer(regs), stack_pointer(regs), ckpt->hdr.checkpoint_arch, ckpt->hdr.is_32bit_application);

	mdbg(INFO4, "Read process registers:");
	debug_registers(regs);

	kfree(platform_regs);

	return 0;
		
	/* error handling */
 exit1:
	kfree(platform_regs);
 exit0:
	return -EINVAL;
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_REGS_PRIVATE


#endif /* TCMI_CKPT_REGS_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_REGS_H */

