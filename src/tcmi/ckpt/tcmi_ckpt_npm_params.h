/**
 * @file tcmi_ckpt_npm_params.h - a helper class that provides functionality to
 *                    non-preemtive migration process parameters (args, env,..)
 */
#ifndef _TCMI_CKPT_NPM_PARAMS_H
#define _TCMI_CKPT_NPM_PARAMS_H

#include "tcmi_ckpt.h"
#include <tcmi/migration/tcmi_npm_params.h>

/** 
 * Writes non-preemptive process startup params to the checkpoint.
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *params - contains current process npm params
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_npm_params_write(struct tcmi_ckpt *ckpt, struct tcmi_npm_params* params) {
	u_int32_t data_length, filename_length = strlen(params->file_name) + 1;
	mdbg(INFO3, "Going to write npm '%s', Args: %d, Envps %d (Filename length: %d, Data length: %d)", params->file_name, params->argsc, params->envpc, filename_length, params->used_data_length);


	if (tcmi_ckpt_write(ckpt, &params->argsc, sizeof(u_int8_t)) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}

	if (tcmi_ckpt_write(ckpt, &params->envpc, sizeof(u_int8_t)) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}

	if (tcmi_ckpt_write(ckpt, &filename_length, sizeof(u_int32_t)) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}

	if (tcmi_ckpt_write(ckpt, params->file_name, filename_length) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}

	data_length = params->used_data_length;

	if (tcmi_ckpt_write(ckpt, &data_length, sizeof(u_int32_t)) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}

	if (tcmi_ckpt_write(ckpt, params->data, data_length) < 0) {
		mdbg(ERR3, "Error writing npm params");
		goto exit0;
	}	
	
	mdbg(INFO4, "Written npm params. Size: %d.", (filename_length+data_length));

	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}

/** 
 * Reads non-preemptive process startup params from the checkpoint
 *
 * @param *ckpt - checkpoint file where the area is to be stored
 * @param *params - This structure will be filled with the npm params
 * @return 0 upon success.
 */
static inline int tcmi_ckpt_npm_params_read(struct tcmi_ckpt *ckpt, struct tcmi_npm_params* params) {
	u_int32_t data_length, filename_length;

	if (tcmi_ckpt_read(ckpt, &params->argsc, sizeof(u_int8_t)) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, &params->envpc, sizeof(u_int8_t)) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, &filename_length, sizeof(u_int32_t)) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, params->file_name, filename_length) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, &data_length, sizeof(u_int32_t)) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	if (tcmi_ckpt_read(ckpt, params->data, data_length) < 0) {
		mdbg(ERR3, "Error reading npm params");
		goto exit0;
	}

	// We have to fixup args pointers in params structure
	fixup_npm_params_pointers(params);

	mdbg(INFO4, "Read npm params. Size: %d.", (filename_length+data_length));
	mdbg(INFO3, "Read npm '%s', Args: %d, Envps %d", params->file_name, params->argsc, params->envpc);

	return 0;
		
	/* error handling */
 exit0:
	return -EINVAL;
}


/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_CKPT_NPM_PARAMS_PRIVATE


#endif /* TCMI_CKPT_REGS_PRIVATE */

/**
 * @}
 */

#endif /* _TCMI_CKPT_NPM_PARAMS_H */

