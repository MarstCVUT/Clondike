#ifndef ARCH_TYPES_H
#define ARCH_TYPES_H

#define CHECKED_UINT64_TO_ULONG(val) \
	({ 							\
		BUG_ON(val > ULONG_MAX); 	\
		val;						\
	})

#endif

#define copy_val(s,d,val) (d)->val = (s)->val

/** Some platform independed structures (32vs64 bits).. suffixed by "_ind" */

struct timeval_ind {
         int64_t          tv_sec;         /* seconds */
         int64_t     tv_usec;        /* microseconds */
} __attribute__((__packed__));


static inline void timeval_to_ind(struct timeval* s, struct timeval_ind* d) {
	copy_val(s,d,tv_sec);
	copy_val(s,d,tv_usec);
}

static inline void timeval_from_ind(struct timeval_ind* s, struct timeval* d) {
	copy_val(s,d,tv_sec);
	copy_val(s,d,tv_usec);
}

struct rusage_ind {
        struct timeval_ind ru_utime;        /* user time used */
        struct timeval_ind ru_stime;        /* system time used */
        int64_t    ru_maxrss;              /* maximum resident set size */
        int64_t    ru_ixrss;               /* integral shared memory size */
        int64_t    ru_idrss;               /* integral unshared data size */
        int64_t    ru_isrss;               /* integral unshared stack size */
        int64_t    ru_minflt;              /* page reclaims */
        int64_t    ru_majflt;              /* page faults */
        int64_t    ru_nswap;               /* swaps */
        int64_t    ru_inblock;             /* block input operations */
        int64_t    ru_oublock;             /* block output operations */
        int64_t    ru_msgsnd;              /* messages sent */
        int64_t    ru_msgrcv;              /* messages received */
        int64_t    ru_nsignals;            /* signals received */
        int64_t    ru_nvcsw;               /* voluntary context switches */
        int64_t    ru_nivcsw;              /* involuntary " */
} __attribute__((__packed__));

static inline void rusage_to_ind(struct rusage* s, struct rusage_ind* d) {
	timeval_to_ind(&s->ru_utime, &d->ru_utime);
	timeval_to_ind(&s->ru_stime, &d->ru_stime);
	copy_val(s,d,ru_maxrss);
	copy_val(s,d,ru_ixrss);
	copy_val(s,d,ru_idrss);
	copy_val(s,d,ru_isrss);
	copy_val(s,d,ru_minflt);
	copy_val(s,d,ru_majflt);
	copy_val(s,d,ru_nswap);
	copy_val(s,d,ru_inblock);
	copy_val(s,d,ru_oublock);
	copy_val(s,d,ru_msgsnd);
	copy_val(s,d,ru_msgrcv);
	copy_val(s,d,ru_nsignals);
	copy_val(s,d,ru_nvcsw);
	copy_val(s,d,ru_nivcsw);
}

static inline void rusage_from_ind(struct rusage_ind* s, struct rusage* d) {
	timeval_from_ind(&s->ru_utime, &d->ru_utime);
	timeval_from_ind(&s->ru_stime, &d->ru_stime);
	copy_val(s,d,ru_maxrss);
	copy_val(s,d,ru_ixrss);
	copy_val(s,d,ru_idrss);
	copy_val(s,d,ru_isrss);
	copy_val(s,d,ru_minflt);
	copy_val(s,d,ru_majflt);
	copy_val(s,d,ru_nswap);
	copy_val(s,d,ru_inblock);
	copy_val(s,d,ru_oublock);
	copy_val(s,d,ru_msgsnd);
	copy_val(s,d,ru_msgrcv);
	copy_val(s,d,ru_nsignals);
	copy_val(s,d,ru_nvcsw);
	copy_val(s,d,ru_nivcsw);
}

