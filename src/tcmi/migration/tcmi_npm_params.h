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

/**
 * Given a npm params structure, this method will fixup args and envp pointers to point to correct places 
 * in the data member.
 */
int fixup_npm_params_pointers(struct tcmi_npm_params* params);

#endif
