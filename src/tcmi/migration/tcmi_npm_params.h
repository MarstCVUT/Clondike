/**
 * @file tcmi_npm_params.h - non-preemtive migration process parameters (args, env,..)
 */
#ifndef _TCMI_NPM_PARAMS_H
#define _TCMI_NPM_PARAMS_H

#include <linux/limits.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/binfmts.h>
#include <asm/page.h>

#ifndef MAX_ARG_PAGES
#define MAX_ARG_PAGES (MAX_ARG_STRLEN/PAGE_SIZE)
#endif

#define NPM_FILENAME_MAX PATH_MAX
#define NPM_DATA_SIZE_MAX (PAGE_SIZE*MAX_ARG_PAGES)

/** 
 * Structure containing data required for performing of non-preemtive restart 
 *
 * The structure contains all required data in itself, the pointers all point to a "data" field.
 * The organization in data field is as follows:
 *   args array (pointer to strings) + null termination
 *   args[0] first arg + null termination
 *   .....
 *   envp array (pointer to envp strings) + null termination
 *  ...
 */
struct tcmi_npm_params {
	/** Name of the file that is being "execve-ed" */
	char file_name[NPM_FILENAME_MAX];
	/** Count of args/envp params */
	u_int8_t argsc, envpc;
	/** Arrays of args and envps.. the actual values are stored in "data" member */
	char** args, **envp;
	/** 
         * This field represents, how many bytes of data field is actually used. (It is used for optimization of checkpoint size)
         * This value is filled, when the args&envp are serialized into the data.
         */
	u_int32_t used_data_length;
	/** Helper field that stores content of args/envp for the serialization */
	char data[NPM_DATA_SIZE_MAX];
}  __attribute__((__packed__));

/** Extracts npm params from user space passed params to a tcmi structure.
 *
 * @param params Structure to be filled with extracted data
 * @returns 0 on success
 */
int extract_tcmi_npm_params(struct tcmi_npm_params* params, const char * filename, char __user *__user *argv, char __user *__user *envp);


/** UGLY UGLY! Following 2 funcs would ideally not be in a header.. this is just a quick hack to made code working when deployed as modules.. better solution required */

/** How many bytes to be reserved for array */
static inline int reserve_array_length(int count) {
	// We have to use size of 64 bit pointer so that it is platform compatible
	return (1+count)*sizeof(int64_t);
}

/**
 * Given a npm params structure, this method will fixup args and envp pointers to point to correct places 
 * in the data member.
 */
static int fixup_npm_params_pointers(struct tcmi_npm_params* params) {
	int i, pos =0;

	// Decode args array pointer
	params->args = (char**)params->data;
	pos += reserve_array_length(params->argsc);
	// Fill in pointers in arg arrays pointer
	for ( i=0; i < params->argsc; i++ ) {
		int len = strlen(params->data+pos);
		
		params->args[i] = params->data + pos;
		pos += (len+1);
	}
	params->args[i] = NULL;

	// Decode envp array pointer
	params->envp = (char**)(params->data + pos);
	pos += reserve_array_length(params->envpc);
	// Fill in pointers in envp arrays pointer
	for ( i=0; i < params->envpc; i++ ) {
		int len = strlen(params->data+pos);
		
		params->envp[i] = params->data + pos;
		pos += (len+1);
	}
	params->envp[i] = NULL;

	return 0;
}
#endif
