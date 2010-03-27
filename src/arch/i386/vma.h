#ifndef ARG_I386_VMA_H
#define ARG_I386_VMA_H

/** Returns 1, if the vma should be ignored by checkpointing componenet */
#define vma_ignore(ckpt, vma) ({ \
	/* i386 has all VMAs relevant */ \
	0; \
})

#endif