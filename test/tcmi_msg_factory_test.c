/**
 * @file tcmi_msg_factory_test.c - Basic test case for the TCMI message factory.
 *                      - ask the factory to make a skeleton message - specify wrong group
 *                      - ask the factory to make a skeleton message - specify proper group
 *                      - dispose the message
 *                      - ask the factory to make a skeleton message - specify any group
 *                      - ask the factory to make a skeleton message - specify any group, set
 *                      error flags in the message ID - message won't be created as there is no
 *                      error rx constructor
 *
 *                      - ask the factory to make a skeleton response message - specify wrong group
 *                      - ask the factory to make a skeleton response message - specify proper group
 *                      - dispose the message
 *                      - ask the factory to make a skeleton response message - specify any group
 *                      - dispose the message
 *                      - ask the factory to make a skeleton response message - specify any group, set
 *                      error flags in the message ID - An error message will be created.
 *                      - dispose the message
 * 
 *
 *
 * Date: 04/11/2005
 *
 * Author: Jan Capek
 *
 *
 * License....
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>

#include <tcmi/lib/tcmi_slotvec.h>
#include <tcmi/comm/tcmi_transaction.h>
#include <tcmi/comm/tcmi_msg_factory.h>
#include <tcmi/comm/tcmi_messages_dsc.h>

#include <dbg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jan Capek");


/**
 * Module initialization
 *
 * @return - 
 */
static int __init tcmi_msg_factory_test_init(void)
{
	struct tcmi_msg *m;


	minfo(INFO1, "lgroup %x, anygrp %x, lastmsg %x, lastprocmsg %x", 
	      TCMI_MSG_GROUP_LAST, TCMI_MSG_GROUP_ANY, TCMI_LAST_MSG_ID, TCMI_LAST_PROCMSG_ID);
	/* skeleton message testing */
	if (!(m = tcmi_msg_factory(TCMI_SKEL_MSG_ID, TCMI_MSG_GROUP_PROC)))
		minfo(ERR1, "Error creating skeleton message, as expected - wrong group");

	if (!(m = tcmi_msg_factory(TCMI_SKEL_MSG_ID, TCMI_MSG_GROUP_MIG)))
		minfo(ERR1, "Error creating skeleton message, shouldn't fail..");
	else {
		tcmi_msg_put(m);
	}

	if (!(m = tcmi_msg_factory(TCMI_MSG_FLG_SET_ERR(TCMI_SKEL_MSG_ID), 
				   TCMI_MSG_GROUP_ANY)))
		minfo(ERR1, "Error creating skeleton response message, as expected" 
		      "- no error rx constructor..");
	else
		tcmi_msg_put(m);

	/* skeleton response message testing */
	if (!(m = tcmi_msg_factory(TCMI_SKELRESP_MSG_ID, TCMI_MSG_GROUP_PROC)))
		minfo(ERR1, "Error creating skeleton response message, as expected - wrong group");

	if (!(m = tcmi_msg_factory(TCMI_SKELRESP_MSG_ID, TCMI_MSG_GROUP_MIG)))
		minfo(ERR1, "Error creating skeleton response message, shouldn't fail..");
	else
		tcmi_msg_put(m);

	if (!(m = tcmi_msg_factory(TCMI_MSG_FLG_SET_ERR(TCMI_SKELRESP_MSG_ID), 
				   TCMI_MSG_GROUP_ANY)))
		minfo(ERR1, "Error creating skeleton message, shouldn't fail.." );
	else {
		tcmi_msg_put(m);
	}

	return 0;
}

/**
 * Module cleanup
 */
static void __exit tcmi_msg_factory_test_exit(void)
{
	minfo(INFO1, "Exiting");
}


module_init(tcmi_msg_factory_test_init);
module_exit(tcmi_msg_factory_test_exit);



