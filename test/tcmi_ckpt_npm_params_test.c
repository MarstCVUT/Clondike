#ifdef APP_NAME
#undef APP_NAME
#endif
#define APP_NAME CKPT_NPM_PARAMS-TEST

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <tcmi/migration/tcmi_npm_params.h>

#include <dbg.h>

MODULE_LICENSE("GPL");

static int __init tcmi_ckpt_npm_params_test_init(void)
{
	mm_segment_t old_fs;
	int res = 0;
	struct tcmi_npm_params* params;
	char* argv[] = { "Arg1", "Arg2", NULL };
	char* envp[] = { "Envp1", NULL }; 

	mdbg(INFO3, "Running npm params test");

	params = vmalloc(sizeof(struct tcmi_npm_params));

	old_fs = get_fs();
	set_fs(KERNEL_DS);	
	
	res = extract_tcmi_npm_params(params, "TestFile", (char __user *__user *)argv, (char __user *__user *)envp);
	if ( res ) {
		mdbg(ERR3, "Failed to extract args");
		goto out;
	}

	if ( strcmp(argv[0], params->args[0]) ) {
		mdbg(ERR3, "Arg 1 invalid. Expected %s but was %s", argv[0], params->args[0]);
		res = -EINVAL;
		goto out;
	}

	if ( strcmp(argv[1], params->args[1]) ) {
		mdbg(ERR3, "Arg 2 invalid. Expected %s but was %s", argv[1], params->args[1]);
		res = -EINVAL;
		goto out;
	}

	if ( params->args[2] != NULL ) {
		mdbg(ERR3, "Arg 3 should be NULL");
		res = -EINVAL;
		goto out;
	}

	if ( strcmp(envp[0], params->envp[0]) ) {
		mdbg(ERR3, "Envp 1 invalid. Expected %s but was %s", envp[0], params->envp[0]);
		res = -EINVAL;
		goto out;
	}

	if ( params->envp[1] != NULL ) {
		mdbg(ERR3, "Envp 2 should be NULL");
		res = -EINVAL;
		goto out;
	}

out:	
	set_fs(old_fs);
	vfree(params);
	return res;
}

static void __exit tcmi_ckpt_npm_params_test_exit(void)
{
}


module_init(tcmi_ckpt_npm_params_test_init);
module_exit(tcmi_ckpt_npm_params_test_exit);
