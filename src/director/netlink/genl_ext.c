#include "genl_ext.h"
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

#include <dbg.h>

struct genl_tx_internal {
	struct list_head link; /* To be enlistable in linked lists */

	struct genl_tx tx;

	/* Response data */
	struct sk_buff* skb;
	struct genl_info info;
	
	u64 start_time;

	int interuptible;
	int done;
};

/* guards access to transactions */
static DEFINE_MUTEX(genl_ext_mutex); 
/* Processes waiting on their transactions to be finished */
static DECLARE_WAIT_QUEUE_HEAD(tx_queue);

/* All registered and not yet processed transactions */
static LIST_HEAD(transactions);

static void genl_ext_lock(void) {
        mutex_lock(&genl_ext_mutex);
}

static void genl_ext_unlock(void) {
        mutex_unlock(&genl_ext_mutex);
}

int genlmsg_unicast_tx(struct sk_buff *skb, u32 pid, struct genl_tx* tx, int interuptible) {
	int res;
	
	/* If the transaction context if provided, we have to register it BEFORE we perform the unicast so that we do not miss the response */
	if ( tx ) {
		struct nlmsghdr *nlh;
		struct genlmsghdr *hdr;
		struct genl_tx_internal* itx =  kmalloc(sizeof(struct genl_tx_internal), GFP_KERNEL);
		if ( !itx )
			return -ENOMEM;
		
		itx->start_time = cpu_clock(smp_processor_id());
		itx->interuptible = interuptible;

		nlh = (struct nlmsghdr *) skb->data;
		hdr = nlmsg_data(nlh);

		/*tx->cmd = hdr->cmd;*/		
		tx->seq = nlh->nlmsg_seq;
		memcpy(&itx->tx,tx, sizeof(struct genl_tx));
		itx->skb = NULL; /* Response buffer, not the request buffer */
		itx->done = 0;

		genl_ext_lock();
		list_add(&itx->link, &transactions);
		genl_ext_unlock();

		mdbg(INFO3,"Generic Tx registered: %d Data len: %d\n",tx->seq, skb->len);
	}
	
	res = genlmsg_unicast(&init_net, skb, pid);
	
	if ( res ) {
	    minfo(ERR3, "Unicast error: %d\n", res);
	}
	
	return res;
}

/* Must be called with transactions lock held */
static struct genl_tx_internal* __find_itx(struct genl_tx* tx) {
	struct genl_tx_internal* tmp_entry;
	struct list_head* tmp;

	list_for_each(tmp, &transactions) {
		tmp_entry = list_entry(tmp, struct genl_tx_internal, link);
		if ( memcmp(&tmp_entry->tx, tx, sizeof(*tx)) == 0 ) {
			return tmp_entry;
		};
	};

	return NULL;
}

int genlmsg_read_response(struct genl_tx* tx, struct sk_buff **skb, struct genl_info *info, int timeout) {
	struct genl_tx_internal* itx;
	int err;
	u64 read_time;

	genl_ext_lock();	
	itx = __find_itx(tx);
	genl_ext_unlock();

	if ( !itx )
		return -EINVAL;
	
	if ( itx->interuptible ) {
	  err = wait_event_interruptible_timeout(tx_queue, itx->done == 1, msecs_to_jiffies(timeout*1000));
	}else {
	  /* If the request is not interruptible, we have to wait for reply even when signal arrives. This is, however, generally fast enough not to cause too much trouble (sub ms). */
	  err = wait_event_timeout(tx_queue, itx->done == 1, msecs_to_jiffies(timeout*1000));	  
	}
	    	
	read_time = cpu_clock(smp_processor_id());
	
	mdbg(INFO3,"Reading response done: %d err: %d. Read took: %llu ms", tx->seq, err, (read_time - itx->start_time)/1000000);

	genl_ext_lock();	
	if ( itx )
		list_del(&itx->link);
	genl_ext_unlock();
	
	if ( err == 0 )
		err = -ETIME; /* Timeout */
	
	if ( err < 0 )
		goto wait_err;	

	*skb = itx->skb;
	memcpy(info,&itx->info, sizeof(struct genl_info));
	kfree(itx);
	return 0;

wait_err:
	/*kfree_skb(*skb);*/
	kfree(itx);
	return err;
}

/* This handler will distribute the message to registered transactions */
static int generic_message_handler(struct sk_buff *skb, struct genl_info *info) {
	struct genl_tx_internal* itx;
	struct genl_tx tx;

	/*tx.cmd = info->genlhdr->cmd;*/
	tx.seq = info->snd_seq;
	mdbg(INFO4,"Generic message handler got: %d", tx.seq);

	genl_ext_lock();	
	itx = __find_itx(&tx);	
	genl_ext_unlock();

	if ( !itx ) {		
		/* No tx registered for the message */
		minfo(ERR1, "Sequence not registered: %d", tx.seq);
		return -EINVAL;
	}

	skb = skb_get(skb); /* Get a reference of skb for further processing */
	itx->skb = skb;
	memcpy(&itx->info, info, sizeof(struct genl_info));
	/* TODO: Do we need wmb here? Or does wake up all make a barrier? */
	itx->done = 1;

	wake_up_all(&tx_queue); /* TODO: Some more scalable waking strategy? Wake up only thread waiting for current itx? */

	return 0;
}

struct genl_ops* genlmsg_register_tx_ops(struct genl_family *family, struct nla_policy* policy, u8 command) {
	int err;
	struct genl_ops* ops;

	ops = kmalloc(sizeof(struct genl_ops), GFP_KERNEL);
	ops->cmd = command;
	ops->flags = 0;
	ops->policy = policy;
	ops->doit = generic_message_handler;
	ops->dumpit = NULL;

	if ( (err=genl_register_ops(family, ops)) )  {
		kfree(ops);
		return NULL;
	}

	return ops;
}
