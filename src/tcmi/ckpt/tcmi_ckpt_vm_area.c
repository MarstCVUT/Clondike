/**
 * @file tcmi_ckpt_vm_area.c - a helper class that provides functionality to
 *                     store a single VM area (memory regions)
 *                      
 * 
 *
 *
 * Date: 05/01/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_ckpt_vm_area.c,v 1.4 2007/09/02 10:53:17 stavam2 Exp $
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

#if defined(__i386__)
#include <asm/highmem.h>
#endif
#include <asm/cacheflush.h>

#define TCMI_CKPT_VM_AREA_PRIVATE
#include "tcmi_ckpt_vm_area.h"

/**
 * \<\<public\>\> Writes a specified memory area into the checkpoint
 * file.  Checks for the requested type - when the light version is
 * required, it will do so only if the area is non-writable and maps a
 * file.  The actual work is then delegated to the light or heavy
 * version of this method.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *vma - the actual VM area that is being processed
 * @param type - requested type of the checkpoint
 * @return 0 upon success.
 */
int tcmi_ckpt_vm_area_write(struct tcmi_ckpt *ckpt, struct vm_area_struct *vma,
			    tcmi_ckpt_vm_area_t type)
{
	int err;
	struct tcmi_ckpt_vm_area_hdr hdr;
	hdr.vm_start = vma->vm_start;
	hdr.vm_end = vma->vm_end;
	hdr.vm_flags = vma->vm_flags;
	hdr.vm_pgoff = vma->vm_pgoff;
	/* light version only when it's non-writable and  maps a file */
	if (type == TCMI_CKPT_VM_AREA_LIGHT && 
	    !(vma->vm_flags & VM_WRITE) && vma->vm_file)
		err = tcmi_ckpt_vm_area_write_l(ckpt, vma, &hdr);
	else
		err = tcmi_ckpt_vm_area_write_h(ckpt, vma, &hdr);
	return err;
}


/**
 * \<\<public\>\> Reads a memory area from the checkpoint file. Reads
 * the VM area header and checks for the requested type. The actual
 * work is then delegated to the light or heavy version of this
 * method.
 *
 * @param *ckpt - checkpoint file where the area is stored
 * @return 0 upon success.
 */
int tcmi_ckpt_vm_area_read(struct tcmi_ckpt *ckpt)
{
	int err = 0;
	struct tcmi_ckpt_vm_area_hdr hdr;
	
	if (tcmi_ckpt_read(ckpt, &hdr, sizeof(hdr)) < 0) {
		mdbg(ERR3, "Error read VM aread header");
		goto exit0;
	}
	mdbg(INFO4, "Reading VMA start:%08llx, end: %08llx, flags: %08llx, pgoff %08llx", 
	     (unsigned long long)hdr.vm_start, (unsigned long long)hdr.vm_end, (unsigned long long)hdr.vm_flags, (unsigned long long)hdr.vm_pgoff);
	/* light version only when it's non-writable and  maps a file */
	if (hdr.type == TCMI_CKPT_VM_AREA_LIGHT)
		err = tcmi_ckpt_vm_area_read_l(ckpt, &hdr);
	else if (hdr.type == TCMI_CKPT_VM_AREA_HEAVY)
		err = tcmi_ckpt_vm_area_read_h(ckpt, &hdr);
	else {
		mdbg(ERR3, "Unrecognized header type %x", hdr.type);
		goto exit0;
	}
	return err;

	/* error handling */
 exit0:
	return -EINVAL;
}


/** @addtogroup tcmi_ckpt_vm_area_class
 *
 * @{
 */

/**
 * \<\<private\>\> Writes a specified memory area into the checkpoint
 * file - light version.  It fills out the rest of the header,
 * extracts the mapped file pathname and stores the header, pathname
 * size and the actual pathname into the checkpoint.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *vma - the actual VM area that is being processed
 * @param *hdr - partially filled VM area header
 * @return 0 upon success.
 */
static int tcmi_ckpt_vm_area_write_l(struct tcmi_ckpt *ckpt, 
				     struct vm_area_struct *vma, 
				     struct tcmi_ckpt_vm_area_hdr *hdr)
{
	/* page for the filepathname */
	unsigned long page;
	char *pathname;

	/* finish the header */
	hdr->type = TCMI_CKPT_VM_AREA_LIGHT;

	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		goto exit0;
	}
	if (IS_ERR(pathname = d_path(&vma->vm_file->f_path, 
				     (char*)page, PAGE_SIZE))) {
		mdbg(ERR3, "Can't resolve pathname for '%s'", 
		     vma->vm_file->f_dentry->d_name.name);
		goto exit1;
	}
	hdr->pathname_size = strlen(pathname) + 1;
	/* write the header and the pathname into the checkpoint */
	if (tcmi_ckpt_write(ckpt, hdr, sizeof(*hdr)) < 0) {
		mdbg(ERR3, "Error writing VM_area header chunk");
		goto exit1;
	}
	/* write the header and the pathname into the checkpoint */
	if (tcmi_ckpt_write(ckpt, pathname, hdr->pathname_size) < 0) {
		mdbg(ERR3, "Error writing pathname chunk");
		goto exit1;
	}

	free_page(page);
	return 0;

	/* error handling */
 exit1:
	free_page(page);
 exit0:
	return -EINVAL;
}

/**
 * \<\<private\>\> Writes a specified memory area into the checkpoint
 * file - heavy version.  It fills out the rest of the header, sets
 * pathname size to 0 (not used). and writes the header into the
 * checkpoint file.
 *
 * It is necessary to align the file to page size. This is required
 * otherwise we wouldn't be able to mmap pages from the file upon
 * restoring the checkpoint.
 *
 * Following algorithm dumps the pages, the idea comes from a regular
 * core dump handler (e.g. in fs/binfmt_elf.c)
 *
 \verbatim
  For each page in the region do:
    - get the page - might trigger pagefaults to load the pages
    - map the page descriptor to a valid linear address (user pages might
    be in high memory)
    - if the page is zero or marked untouched, advance the offset in the
    checkpoint only - there is no need to write 0's to disk, just create
    the gap for them.
    - otherwise write the entire page into the checkpoint file.
 \endverbatim
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *vma - the actual VM area that is being processed
 * @param *hdr - partially filled VM area header
 * @return 0 upon success.
 */
static int tcmi_ckpt_vm_area_write_h(struct tcmi_ckpt *ckpt, 
				     struct vm_area_struct *vma, 
				     struct tcmi_ckpt_vm_area_hdr *hdr)
{
	/* page for the filepathname */
	unsigned long addr;

	/* finish the header */
	hdr->type = TCMI_CKPT_VM_AREA_HEAVY;
	hdr->pathname_size = 0;
	/* write the header into the checkpoint */
	if (tcmi_ckpt_write(ckpt, hdr, sizeof(*hdr)) < 0) {
		mdbg(ERR3, "Error writing VM area header chunk");
		goto exit0;
	}
	/* align the current file position to page size boundary */
	tcmi_ckpt_page_align(ckpt);
	for (addr = vma->vm_start; addr < vma->vm_end; addr += PAGE_SIZE) {
		struct page *page;
		struct vm_area_struct *vma;
		void *kaddr;
		if (get_user_pages(current, current->mm, addr, 1, 0, 1,
				   &page, &vma) <= 0) {
			mdbg(INFO4, "Skipping untouched page at %08lx", addr);
			tcmi_ckpt_seek(ckpt, PAGE_SIZE, 1);
			continue;
		} 
		/*if (page == ZERO_PAGE(addr)) {
			mdbg(INFO4, "Skipping zero page at %08lx", addr);
			tcmi_ckpt_seek(ckpt, PAGE_SIZE, 1);
			continue;
		}
		*/
		/* actual page, that needs to be written. */
		flush_cache_page(vma, addr,  page_to_pfn(page));  /* TODO: check this fix is correct */
		kaddr = tcmi_ckpt_vm_area_kmap(page);
		/* write the page into the checkpoint */
		if (tcmi_ckpt_write(ckpt, kaddr, PAGE_SIZE) < 0) {
			mdbg(ERR3, "Error writing page at %08lx", addr);
			goto exit0;
		}
		tcmi_ckpt_vm_area_kunmap(page);
	}
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}



/** 
 * \<\<private\>\> Reads a memory area from the checkpoint file -
 * light version. Reads the pathname of the file that is to be mapped
 * into the area.
 * - opens the file 
 * - performs the memory mapping, 
 * - releases the file - the memory mapping retains its own reference.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *hdr - VM area header
 * @return 0 upon success.
 */

static int tcmi_ckpt_vm_area_read_l(struct tcmi_ckpt *ckpt, 
				    struct tcmi_ckpt_vm_area_hdr *hdr)
{
	/* page for the filepathname */
	unsigned long page;
	char *pathname = NULL;
	struct file *file; /* file object for the vm_file */
	unsigned long mmap_flags;
	unsigned long addr;

	/* finish the header */
	hdr->type = TCMI_CKPT_VM_AREA_LIGHT;

	/* resolve the path name. */
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for file pathname!");
		goto exit0;
	}
	pathname = (char*) page;
	/* read the header and the pathname from the checkpoint */
	if (tcmi_ckpt_read(ckpt, pathname, hdr->pathname_size) < 0) {
		mdbg(ERR3, "Error reading pathname");
		goto exit1;
	}
	/* convert the VM flags to MMAP flags */
	mmap_flags = tcmi_ckpt_vm_area_to_mmap_flags(hdr->vm_flags);

	file = filp_open(pathname, O_RDONLY, 0); 
	if (IS_ERR(file)) {
		mdbg(ERR3, "Error opening file '%s'", pathname);
		goto exit1;
	}
	/* prot flags are just the lower bits extracted from vm_flags - see mman.h, mm.h */
	addr = do_mmap_pgoff(file, hdr->vm_start, hdr->vm_end - hdr->vm_start,
			     hdr->vm_flags & (PROT_READ | PROT_EXEC| PROT_WRITE),	
			     mmap_flags, hdr->vm_pgoff);

	if (addr != hdr->vm_start) {
		mdbg(ERR3, "Error mapping file '%s' at: %lx", pathname, addr);
		goto exit2;
	}

	/* or shall we perform filp_close??? */
	fput(file);
	free_page(page);

	mdbg(INFO4, "Mapped at: 08%lx, VM_flags = %08llx, mmap flags = %08lx", 
	     addr, (unsigned long long)hdr->vm_flags, mmap_flags);
	return 0;

	/* error handling */
 exit2:
	fput(file);
 exit1:
	free_page(page);
 exit0:
	return -EINVAL;
	
}

/** 
 * \<\<private\>\> Reads a memory area from the checkpoint file -
 * heavy version.
 * - aligns the position in the file to page boundary
 * - computes memory mapping flags.
 * - performs the memory mapping.
 * There is special case that needs to be handled - if the area
 * grows down which indicates a stack region stack setup
 * work is delegated to tcmi_ckpt_vm_area_stack_fixup().
 *
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *hdr - VM area header
 * @return 0 upon success.
 */
static int tcmi_ckpt_vm_area_read_h(struct tcmi_ckpt *ckpt, 
				    struct tcmi_ckpt_vm_area_hdr *hdr)
{
	unsigned long mmap_flags;
	unsigned long addr;
	unsigned long length;

	tcmi_ckpt_page_align(ckpt);

	length = hdr->vm_end - hdr->vm_start;

	if ( length < PAGE_SIZE ) {
		mdbg(ERR3, "Length of the area shorter than one page. Start: %16llx End: %16llx", (unsigned long long)hdr->vm_start, (unsigned long long)hdr->vm_end);
		goto exit0;
	}	

	/* check if stack fixup is needed */
	if ((hdr->vm_flags & VM_GROWSDOWN) ) {
	    	if (tcmi_ckpt_vm_area_stack_fixup(ckpt, hdr) < 0) {
			mdbg(ERR3, "Failed setting up stack area");
			goto exit0;
		}

		length -= PAGE_SIZE;

		/* The are was fully maped by the stack fixup */
		if ( length == 0 )
			return 0;

	}

	/* convert the VM flags to MMAP flags */
	mmap_flags = tcmi_ckpt_vm_area_to_mmap_flags(hdr->vm_flags);
	/* prot flags are just the lower bits extracted from vm_flags - see mman.h, mm.h */
	addr = tcmi_ckpt_do_mmap(ckpt, hdr->vm_start, hdr->vm_end - hdr->vm_start,
				 hdr->vm_flags & (PROT_READ | PROT_EXEC| PROT_WRITE),	
				 mmap_flags);
	if (addr != hdr->vm_start) {
		mdbg(ERR3, "Error mapping checkpoint file at: %08lx (hdr->vm_start: %16llx", addr, (unsigned long long)hdr->vm_start);
		goto exit0;
	}
	mdbg(INFO4, "Mapped checkpoint at: %08lx, VM_flags = %16llx, mmap flags = %08lx", 
	     addr, (unsigned long long)hdr->vm_flags, mmap_flags);
	return 0;

	/* error handling */
 exit0:
	return -EINVAL;
}


/**
 * \<\<private\>\> This method is responsible for fixing a the stack
 * in the area just read from the checkpoint file. The idea is the
 * split the supplied VM area into two:
 *
 * - one containing the top of the stack
 * - the other will be created by the standard mechanism.
 * 
 * What needs to be done is to:
 *
 * - create an anonymous mapping with the MAP_GROWSDOWN flag, with start
 * address the same as the VM area in the checkpoint and size of one page.  
 * - read the first page of the stack area from the checkpoint file
 * and copy it into the newly created mapping. The current top of the
 * stack is somewhere in this page.
 * - adjust the VM area header read from the checkpoint file:
 * increment vm_start address and reset the VM_GROWSDOWN flag.
 *
 * Why is it so complex? Stack expansion is possible only for areas
 * marked by VM_GROWSDOWN flag. However, it is not possible to create
 * a file memory mapping using MAP_GROWSDOWN.  The memory mapping
 * framework simply denies this as a regular user doesn't need it.  To
 * change the size of a memory mapping, all processes are encouraged
 * to use mremap().  Further, we would have a problem when the current
 * stack pointer leaves the memory region by the allowed limit(max
 * 32bytes). Where would the expansion get the page? It might end up
 * asking the file to supply the missing page. This is obviously not
 * possible.
 *
 *
 * @param *ckpt - checkpoint file where the current stack area resides.
 * @param *hdr - VM area header
 * @return 0 upon success.
 */
static int tcmi_ckpt_vm_area_stack_fixup(struct tcmi_ckpt *ckpt, 
					 struct tcmi_ckpt_vm_area_hdr *hdr)
{
	/* temporary page allocated for the top of the stack */
	unsigned long page;
	/* address of the target mapping - must match hdr->vm_start */
	unsigned long addr;
	unsigned long mmap_flags;

	/* convert the VM flags to MMAP flags */
	mmap_flags = tcmi_ckpt_vm_area_to_mmap_flags(hdr->vm_flags);

	/* prot flags are just the lower bits extracted from vm_flags - see mman.h, mm.h */
	down_write(&current->mm->mmap_sem);
	addr = do_mmap(NULL, hdr->vm_start, PAGE_SIZE,
		       hdr->vm_flags & (PROT_READ | PROT_EXEC| PROT_WRITE),	
		       mmap_flags, 0);
	up_write(&current->mm->mmap_sem);

	if (addr != hdr->vm_start) {
		mdbg(ERR3, "Error creating an anonymous mapping for stack page %d", (int)addr);
		goto exit0;
	}
	
	if (!(page = __get_free_page(GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate page for stack!");
		goto exit0;
	}
	/* read the first page of the stack */
	if (tcmi_ckpt_read(ckpt, (void*)page, PAGE_SIZE) < 0) {
		mdbg(ERR3, "Error stack page!");
		goto exit1;
	}
	mdbg(INFO4, "Copying stack page to %08lx from %08lx", addr, page);
	if (tcmi_ckpt_vm_area_copy_page(addr, page) < 0) {
		mdbg(ERR3, "Failed to copy stack page!!!");
		goto exit1;
	}
	/* memcpy((void*)addr, (void*)page, PAGE_SIZE); */
	mdbg(INFO4, "Copying done");
	/* adjust the attributes */
	hdr->vm_start += PAGE_SIZE;
	hdr->vm_flags &= ~VM_GROWSDOWN; /* FIXME: make this unified for all archs!!!*/

	free_page(page);

	mdbg(INFO4, "Created anonymous mapping for stack at: %08lx", addr);
	return 0;

	/* error handling */
 exit1:
	free_page(page);
 exit0:
	return -EINVAL;
}

/**
 * @}
 */

