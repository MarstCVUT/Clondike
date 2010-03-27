#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>

#include <dbg.h>
#include <proxyfs/proxyfs_fs.h>
#include <proxyfs/proxyfs_server.h>

/** Reference to proxy server task */
static struct proxyfs_task* proxy_server_task;

static char *server = "no_server";
module_param(server, charp, S_IRUGO);


static int proxyfs_module_init(void) {
	mdbg(INFO3, "ProxyFS loading");

	if( strncmp(server, "no_server", 20) != 0 ){
		mdbg(INFO3, "Request to start server on %s", server);
		if ( (proxy_server_task = proxyfs_task_run( proxyfs_server_thread, server )) == NULL ){
			mdbg(ERR1, "Failed to start server on %s", server);
			return -EINVAL;
		}
	}

	return proxyfs_fs_init();
}

static void proxyfs_module_exit(void) {
	mdbg(INFO3, "ProxyFS unloading");

	proxyfs_task_put(proxy_server_task);
	proxyfs_fs_exit();
	mdbg(INFO3, "ProxyFS unloaded");
}

module_init(proxyfs_module_init);
module_exit(proxyfs_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Petr Malat");
