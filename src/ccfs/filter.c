#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/pagemap.h>
#include <linux/pagevec.h>

#include <dbg.h>

#include "filter.h"

struct filter_pattern {
	/** For enlistment */
	struct list_head head;
	/** Pattern itself */
	char* pattern;
	int pattern_length;
	/** Files creation data has to be older than this time */
	struct timespec older_than;
};

struct filter* create_filter(void) {
	struct filter* filter = kmalloc(sizeof(struct filter), GFP_KERNEL);
	if ( !filter )
		return NULL;

	INIT_LIST_HEAD(&filter->patterns);

	return filter;
}

void destroy_filter(struct filter* filter) {
	struct filter_pattern *head, *tmp;
	if ( !filter )
		return;

        list_for_each_entry_safe(head, tmp, &filter->patterns, head) {			
                list_del(&head->head);
		kfree(head);
		 
	}

	kfree(filter);
}

void parse_filter(struct filter* filter, char* definition) {
	char* p;

	while ((p = strsep(&definition, ":")) != NULL) {
		if (!*p)
			continue;

		add_filter_pattern(filter, p, strcspn(p, ":"));
	}
	
}

int add_filter_pattern(struct filter* filter, const char* pattern, int pattern_length) {
	struct filter_pattern* filter_pattern = kmalloc(sizeof(struct filter_pattern), GFP_KERNEL);

	printk("Adding filter pattern: %s (%d)\n", pattern, pattern_length);
	if ( !filter_pattern )
		return -ENOMEM;

	if ( strcmp(pattern, "OLD") == 0 ) {
	  filter_pattern->pattern = NULL;
	  filter_pattern->older_than = CURRENT_TIME;
	  // Consider also files created in last hour to be new files.. as we may assume they may change again soon (not realy smart but should be good enough for our purposes)	 
	  filter_pattern->older_than.tv_sec = filter_pattern->older_than.tv_sec - 3600;
	} else {	
	  filter_pattern->pattern = kmalloc(pattern_length, GFP_KERNEL);
	  if ( !filter_pattern->pattern ) {
		  kfree(filter_pattern);
		  return -ENOMEM;
	  }
	  
	  memcpy(filter_pattern->pattern, pattern, pattern_length);
	  filter_pattern->pattern_length = pattern_length;		  
	}

	list_add(&filter_pattern->head, &filter->patterns);

	return 0;
}

static int pattern_matches(struct filter_pattern* filter_pattern, const char* name, int name_length, struct timespec* ctime, umode_t umode) {
	int name_index = name_length - 1;
	int pattern_index = filter_pattern->pattern_length - 1;
	const char* pattern = filter_pattern->pattern;

	if ( filter_pattern->pattern == NULL ) {
	  /* Match time pattern. Do not check age for dir, they can be always cached since readdir is uncached anyway */
	  mdbg(INFO3, "Date compare: %d vs %d", &filter_pattern->older_than.tv_sec, ctime->tv_sec);
	  return S_ISDIR(umode) || timespec_compare(&filter_pattern->older_than, ctime) > 0;
	} else {		  	
	  /* Match string pattern */
	  while ( name_index > -1 && pattern_index > -1 ) {
		  if ( pattern[pattern_index] == '*' ) {
			  if ( pattern_index != 0 ) {
				  printk(KERN_WARNING "Illegal pattern '%s'! Pattern must have * only at the beggining!\n", filter_pattern->pattern);
			  }
			  return 1;
		  } else if ( pattern[pattern_index] != name[name_index] ) {
			  return 0;
		  }

		  name_index--;
		  pattern_index--;
	  }
	}

	return name_index == pattern_index;
}

/** Returns 1, if name matches any of the filter patterns. 0 otherwise */
int filter_matches(struct filter* filter, const char* name, int name_length, struct timespec *ctime, umode_t umode) {
	struct filter_pattern *head;

	if ( !filter )
		return 1;

        list_for_each_entry(head, &filter->patterns, head) {			
		 if ( pattern_matches(head, name, name_length, ctime, umode) )
			return 1;
	}

	return 0;
}
