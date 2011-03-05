#ifndef FILTER_H
#define FILTER_H


/**
 * Container of filters. Used for pattern matching of cacheable files.
 */
struct filter {
	/** 
         * Pattterns, that can be matched.
	 * Currently only full name or * wildcard at the beginning are supported.
         */
	struct list_head patterns;
};

struct filter* create_filter(void);
void destroy_filter(struct filter* filter);
/** Registers a new patter to the filter. Returns 0 upon succes, error code otherwise */
int add_filter_pattern(struct filter* filter, const char* pattern, int pattern_length);

/** Parses filter definition. Expects colon separated patterns */
void parse_filter(struct filter* filter, char* definition);

/** Returns 1, if name matches any of the filter patterns. 0 otherwise */
int filter_matches(struct filter* filter, const char* name, int name_length, struct timespec *ctime, umode_t umode);

#endif
