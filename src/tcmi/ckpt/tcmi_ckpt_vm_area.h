/**
 * @file tcmi_ckpt_vm_area.h - a helper class that provides functionality to
 *                     store a single VM area (memory regions)
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_vm_area.h,v 1.3 2007/09/02 10:53:17 stavam2 Exp $
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
#ifndef _TCMI_CKPT_VM_AREA_H
#define _TCMI_CKPT_VM_AREA_H

#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/hardirq.h>

#include "tcmi_ckpt.h"

/** @defgroup tcmi_ckpt_vm_area_class tcmi_ckpt_vm_area class 
 *
 * @ingroup tcmi_ckpt_class
 *
 * This is a \<\<singleton\>\> that takes care of (re)storing a
 * particular memory segment (VM area) into the checkpoint file.
 *
 * There are 2 modes supported:
 * - heavy checkpoint mode - where the entire segment is dumped
 * page by page into the checkpoint. 
 * - light checkpoint mode - if the segment maps a file on disk
 * and the segment is non-writable, only file pathname is dumped
 * into the checkpoint. This decreases the resulting checkpoint 
 * size dramatically. Also, when restoring such checkpoint parts,
 * there is a big chance that the file is already mapped in memory.
 * This typically happens with shared libraries and executable code.
 *
 * @{
 */

/** Describes the type of the vm area we store, see above. */
typedef enum {
	TCMI_CKPT_VM_AREA_LIGHT,
	TCMI_CKPT_VM_AREA_HEAVY
} tcmi_ckpt_vm_area_t;

/** Compound structure describes a particular memory region. Very
 * similar to vm_area_struct. We don't store the vm_page_prot
 * flags as they can be derived from vm_flags */
struct tcmi_ckpt_vm_area_hdr {
	/** segment start address */
	u_int64_t vm_start;
	/** first linear address after the memory region */
	u_int64_t vm_end;
	/** flags of the region */
	u_int64_t vm_flags;
	/** Offset in mapped file, if any. */
	u_int64_t vm_pgoff;
	/** type of the area */
	tcmi_ckpt_vm_area_t type;
	/** pathname size in bytes, including trailing zero - for
	 * light version only */
	u_int32_t pathname_size;
}  __attribute__((__packed__));


/** \<\<public\>\> Writes a specified memory area into the checkpoint file. */
extern int tcmi_ckpt_vm_area_write(struct tcmi_ckpt *ckpt, 
				   struct vm_area_struct *vma, 
				   tcmi_ckpt_vm_area_t type);

/**\<\<public\>\>  Reads a memory area from the checkpoint file. */
extern int tcmi_ckpt_vm_area_read(struct tcmi_ckpt *ckpt);

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_VM_AREA_PRIVATE

/** Reads a memory area from the checkpoint file - light version. */
static int tcmi_ckpt_vm_area_read_l(struct tcmi_ckpt *ckpt, 
				    struct tcmi_ckpt_vm_area_hdr *hdr);

/** Reads a memory area from the checkpoint file - heavy version. */
static int tcmi_ckpt_vm_area_read_h(struct tcmi_ckpt *ckpt, 
				    struct tcmi_ckpt_vm_area_hdr *hdr);

/** Writes a specified memory area into the checkpoint file - light
 * version. */
static int tcmi_ckpt_vm_area_write_l(struct tcmi_ckpt *ckpt, 
				     struct vm_area_struct *vma, 
				     struct tcmi_ckpt_vm_area_hdr *hdr);

/** Writes a specified memory area into the checkpoint file - heavy
 * version. */
static int tcmi_ckpt_vm_area_write_h(struct tcmi_ckpt *ckpt, 
				     struct vm_area_struct *vma, 
				     struct tcmi_ckpt_vm_area_hdr *hdr);

/** 
 * \<\private\>\> Converts the VM area flags to mmap flags.
 *
 * Each mapping is marked fixed as that is the only way how to restart
 * a process from the checkpoint (virtual addresses must match).
 *
 * VM_GROWSDOWN, VM_DENYWRITE, VM_EXECUTABLE flags directly match mmap
 * flags, so only bit extraction needs to be done.  VM_GROWSDOWN flags
 * is special as it marks the stack. Stack fixup is handled separate by
 * tcmi_ckpt_vm_area_stack_fixup().
 *
 * VM_SHARED flag doesn't match the mmap flags directly, so a
 * conditional check needs to be done: anything not shared is mmaped
 * as private.
 *
 * @param vm_flags - flags to be converted
 * @return mmap flags
 */
static inline unsigned long tcmi_ckpt_vm_area_to_mmap_flags(unsigned long vm_flags) 
{
  return MAP_FIXED |
    (vm_flags & VM_SHARED ? MAP_SHARED : MAP_PRIVATE) |
    (vm_flags & VM_GROWSDOWN) |
    (vm_flags & VM_DENYWRITE) |
    (vm_flags & VM_EXECUTABLE);
}


/**
 * \<\private\>\> This is really stupid, but linux kernel doesn't
 * export kmap/kunmap, so we had to duplicate the highmemory check..
 *
 * @param *page - page that is to be mapped into the linear address space
 * @return linear address of the page
 */
static inline void* tcmi_ckpt_vm_area_kmap(struct page *page)
{
        might_sleep();

	#if defined(__i386__)
		if (!PageHighMem(page))
			return page_address(page);
		return kmap_high(page);
	#else
		return page_address(page);
	#endif
}


/**
 * \<\private\>\> Same problem is before..  We had to duplicate the
 * highmemory check..
 *
 * @param *page - page that is to be unmapped
 */
static inline void tcmi_ckpt_vm_area_kunmap(struct page *page)
{
	if (in_interrupt())
                BUG();

	#if defined(__i386__)
		if (!PageHighMem(page))
			return;
		kunmap_high(page);
	#endif
}

/**
 * \<\<private\>\> Helper method that provides a safe mechanism of
 * copying a page from kernel into the address space of a process that
 * is being restored. For some reason we can't perform a plain copy
 * (e.g. using copy_to_user()). It fails on AMD, no idea why. The page
 * fault handler should be able to put up with the generated
 * exception. However on AMD, something makes the kernel atomic, which
 * is a bad thing to have in pagefault handler (see
 * arch/i386/mm/fault.c). Below is a sample oops that occurs on
 * copying without this mechanism:
 *
 \verbatim
 Unable to handle kernel paging request at virtual address bffeb000
  printing eip:
 c026be4f
 *pde = 00000000
 Oops: 0002 [#1]
 PREEMPT
 Modules linked in: coda tcmickptcom lp iptable_filter ipt_MASQUERADE ...
 CPU:    0
 EIP:    0060:[_mmx_memcpy+111/384]    Not tainted VLI
 EFLAGS: 00010212   (2.6.11.7)
 EIP is at _mmx_memcpy+0x6f/0x180
 eax: bffeb000   ebx: 00000040   ecx: 00000001   edx: c9172000
 esi: c901d000   edi: bffeb000   ebp: 00001000   esp: c9173dd4
 ds: 007b   es: 007b   ss: 0068
 Process testcheckpoint (pid: 3983, threadinfo=c9172000 task=c96706b0)
 Stack: bffeb000 c0000000 c901d000 bffeb000 c9172000 d0dd1670 bffeb000 c901d000
        00001000 c901d000 00000112 000bffeb c9172000 c0000000 c9173e6c c9695570
        d0dd14d2 c9695570 c9173e6c 00000000 00000000 c9173e58 c017a448 c9b40e68
 Call Trace:
  [pg0+277272176/1068082176] tcmi_ckpt_vm_area_stack_fixup+0x170/0x204 [tcmickptcom]
  [pg0+277271762/1068082176] tcmi_ckpt_vm_area_read_h+0x242/0x270 [tcmickptcom]
  [vfs_read+184/288] vfs_read+0xb8/0x120
  [pg0+277269799/1068082176] tcmi_ckpt_vm_area_read+0xe7/0x100 [tcmickptcom]
  [pg0+277266764/1068082176] tcmi_ckpt_read_vmas+0x1c/0x40 [tcmickptcom]
  [pg0+277264592/1068082176] tcmi_ckptcom_restart+0x300/0x630 [tcmickptcom]
  [__alloc_pages+754/1120] __alloc_pages+0x2f2/0x460
  [copy_from_user+106/176] copy_from_user+0x6a/0xb0
  [pg0+277263824/1068082176] tcmi_ckptcom_restart+0x0/0x630 [tcmickptcom]
  [search_binary_handler+406/752] search_binary_handler+0x196/0x2f0
  [do_execve+401/560] do_execve+0x191/0x230
  [sys_execve+61/512] sys_execve+0x3d/0x200
  [syscall_call+7/11] syscall_call+0x7/0xb
 Code: 86 c0 00 00 00 0f 0d 86 00 01 00 00 83 fb 05 7e 59 8b 44 24 18 0f 0d 86 40 01 00 00 0f 6f 06 0f 6f 4
e 08 0f 6f 56 10 0f 6f 5e 18 <0f> 7f 00 0f 7f 48 08 0f 7f 50 10 0f 7f 58 18 0f 6f 46 20 0f 6f
  <6>note: testcheckpoint[3983] exited with preempt_count 1
 Debug: sleeping function called from invalid context at include/linux/rwsem.h:43
 in_atomic():1, irqs_disabled():0
 \endverbatim
 *
 * This happens when restoring the very first stack page (see
 * tcmi_ckpt_vm_area_stack_fixup()). The area has been of course
 * mmapped prior to issuing memcpy. Something has set the kernel
 * atomic, that's why the pagefault handler oopses. It occurs on AMD
 * (tested on a Duron laptop and an Athlon XP workstation)
 *
 *
 * @param dst_addr - destination address in user space
 * @param src_addr - source address in kernel space
 * @return 0 upon success
 * @todo Investigate the AMD related issue in page fault handler.
 */
static inline int tcmi_ckpt_vm_area_copy_page(unsigned long dst_addr, unsigned long src_addr)
{
	struct page *dst_page;
	struct vm_area_struct *vma;
	void *kaddr;

	if (get_user_pages(current, current->mm, dst_addr, 1, 1, 1,
			   &dst_page, &vma) <= 0) {
		mdbg(ERR3, "Failed to get a user page at %08lx", dst_addr);
		goto exit0;
	} 
	/* actual page, that needs to be written. */
	kaddr = tcmi_ckpt_vm_area_kmap(dst_page);
	memcpy(kaddr, (void*)src_addr, PAGE_SIZE);
	tcmi_ckpt_vm_area_kunmap(dst_page);
	return 0;
	
	/* error handling */
 exit0:
	return -EINVAL;
}

/** Performs a stack setup based on the area just read from the
 * checkpoint file. */
static int tcmi_ckpt_vm_area_stack_fixup(struct tcmi_ckpt *ckpt, 
					 struct tcmi_ckpt_vm_area_hdr *hdr);

#endif /* TCMI_CKPT_VM_AREA_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_VM_AREA_H */

