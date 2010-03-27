#ifndef PREFETCHER_H
#define PREFETCHER_H

/** Initializes prefetching service, returns 0 upon success */
int initialize_prefetcher(void);

/** Synchronously terminates prefetching service */
void finalize_prefetcher(void);

/** 
 * Submits file, whose content should be fully prefetched.
 * One file reference should be hold by the caller and this reference should be left to the prefetcher,
 * who will free it after it is done with the prefetch.
 */
void submit_for_prefetch(struct file* filp);

#endif
