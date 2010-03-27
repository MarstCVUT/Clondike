/**
 * @file tcmi_method_wrapper.h - artificial class that is used when 
 *                               submitting object methods
 * 
 *
 *
 * Date: 04/25/2005
 *
 * Author: Jan Capek
 *
 * $Id: tcmi_method_wrapper.h,v 1.3 2007/10/07 15:54:00 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2005  Jan Capek
 * 
 * TCMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TCMI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef _TCMI_METHOD_WRAPPER_H
#define _TCMI_METHOD_WRAPPER_H

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/atomic.h>

#include <dbg.h>

/** @defgroup tcmi_method_wrapper_class tcmi_method_wrapper class 
 * 
 * @ingroup tcmi_task_group
 * 
 * This artificial class is used for queueing methods to an arbitrary
 * object. It contains the method to be called and a node to link
 * the instance into a queue. In addition, there is data stored inside
 * the wrapper. The wrapper instance itself is passed as the method
 * argument, so that target object can extract the data.
 *
 * @{
 */

/** Forward declaration */
struct tcmi_method_wrapper;

/** Describes the method that can be submitted into the method queue. */
typedef int tcmi_method_t(void*, struct tcmi_method_wrapper*);

/** Compound structure that describes the method. */
struct tcmi_method_wrapper {
	/** node to link the wrapper in a queue. */
	struct list_head node;
	/** method stored in the wrapper. */
	tcmi_method_t *method;
	/** data passed as an argument to the method. */
	void *data;

	/** Instance reference counter. */
	atomic_t ref_count;
};

/**
 * \<\<public\>\> Creates a new wrapper for the specified method. If
 * the data argument is non-NULL, it will also allocate space for the
 * data and copy 'size' bytes over.
 *
 * @param method - method that the wrapper holds
 * @param data - data that is used as method argumet
 * @param size - size of the data 
 * @return new method wrapper instance or NULL
 */
static inline struct tcmi_method_wrapper* tcmi_method_wrapper_new(tcmi_method_t *method, 
								  void *data, int size)
{
	struct tcmi_method_wrapper *wrapper;

	if (!(wrapper = (struct tcmi_method_wrapper*)kmalloc(sizeof(struct tcmi_method_wrapper), 
							     GFP_KERNEL))) {
		mdbg(ERR3, "Can't allocate memory for the wrapper");
		goto exit0;
	}
	INIT_LIST_HEAD(&wrapper->node);
	wrapper->method = method;
	wrapper->data = NULL;
	/* copy the data over if any specified. */
	if (data && size) {
		if (!(wrapper->data = vmalloc(size))) {
			mdbg(ERR3, "Can't allocate memory for the wrapper data");
			goto exit1;
		}
		memcpy(wrapper->data, data, size);
	}
	atomic_set(&wrapper->ref_count, 1);

	mdbg(INFO4, "Created method wrapper %p", wrapper);
	return wrapper;

	/* error handling */
 exit1:
	kfree(wrapper);
 exit0:
	return NULL;
}

/** 
 * \<\<public\>\> Method wrapper accessor. Increments the reference
 * counter
 * 
 * @param *self - pointer to this method wrapper instance
 * @return - self
 */
static inline struct tcmi_method_wrapper* tcmi_method_wrapper_get(struct tcmi_method_wrapper *self)
{
	if (!self)
		return NULL;
	atomic_inc(&self->ref_count);
	return self;
}

/**
 * \<\<public\>\> Decrements the reference counter, when the count
 * reaches 0, the instance is destroyed.
 *
 * @param *self - pointer to this method wrapper instance
 */
static inline void tcmi_method_wrapper_put(struct tcmi_method_wrapper *self)
{
	if (!self)
		return;

	if (atomic_dec_and_test(&self->ref_count)) {
		list_del_init(&self->node);
		mdbg(INFO4, "Destroying method wrapper %p", self);
		vfree(self->data);
		kfree(self);
	}

}

/**
 * \<\<public\>\> Calls the method that resides in the wrapper. The
 * target object is specified as the parameter
 *
 * @param *self - pointer to this method wrapper instance
 * @param *obj - object whose method is to be called
 * @return status of the called method
 */
static inline int tcmi_method_wrapper_call(struct tcmi_method_wrapper *self, void *obj)
{
	int ret = 0;

	if (self && self->method) {
		ret = self->method(obj, self);
	}
	return ret;

}

/**
 * \<\<public\>\> Accessor for the data stored in the method wrapper.
 *
 * @param *self - pointer to this method wrapper instance
 * @return pointer to the data
 */
static inline void* tcmi_method_wrapper_data(struct tcmi_method_wrapper *self)
{
	return self->data;
}
/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_METHOD_WRAPPER_PRIVATE
#endif /* TCMI_METHOD_WRAPPER_PRIVATE */


/**
 * @}
 */


#endif /* _TCMI_METHOD_WRAPPER_H */
