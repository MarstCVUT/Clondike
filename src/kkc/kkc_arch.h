/** @file kkc_arch.h - KKC architecture descriptor class.
 *
 * Date: 04/13/2005
 *
 * Author: Jan Capek
 *
 * $Id: kkc_arch.h,v 1.2 2007/07/02 02:26:17 malatp1 Exp $
 *
 * This file is part of Kernel-to-Kernel Communication Library(KKC)
 * Copyleft (C) 2005  Jan Capek
 * 
 * KKC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * KKC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _KKC_ARCH_H
#define _KKC_ARCH_H

#include <linux/module.h>
#include <linux/string.h>
#include <linux/stringify.h>

#include <dbg.h>

/** @defgroup kkc_arch_class kkc_arch class
 *
 * @ingroup kkc_class
 *
 * This class describes a particular network architecture. Each
 * architecture has its unique name. Based on this name, the main KKC
 * module will lookup a particular architecture to create a user
 * specified object. 
 *
 * The instances of kkc_arch are to be created at compile time by the
 * corresponding architecture socket class(e.g. \link
 * kkc_sock_tcp_class kkc_sock_tcp \endlink). The socket class then
 * provides architecture specific operations. Currently, there is only
 * one operation needed - the socket constructor.
 * 
 * Also each particular architecture instance retains a reference to a
 * module where it has been declared. Each object created via the
 * architecture owns a reference to the architecture. That way, the
 * module can't get unloaded when instance of its sockets still exist.
 *
 * @{
 */
/** Maximum architecture string length. */
#define KKC_MAX_ARCH_LENGTH 20

/** Describes a generic KKC architecture object */
typedef void* kkc_arch_obj_t;

/** Architecture specific operations */
struct kkc_arch_ops {
	/** instantiates a KKC object. */
	kkc_arch_obj_t (*kkc_arch_obj_new)(void);
};


/** Compound structure, describes one particular architecture. */
struct kkc_arch {
	/** Name of network architecture (e.g. 'tcp'). */
	char name[KKC_MAX_ARCH_LENGTH];
	
	/** Architecture specific operations. */
	struct kkc_arch_ops *arch_ops;

	/** Module owning this architecture. */
	struct module *owner;

	/** Any data needed by particular network architecture. */
	void *data;
};


/** KKC architecture prefix. */
#define KKC_ARCH_PREFIX kkc_arch_
/** KKC architecture prefix in string form - used when looking up
 * particular architecture. */
#define KKC_ARCH_PREFIX_STR __stringify(KKC_ARCH_PREFIX)

/** Constructs the architecture symbol name. */
#define KKC_ARCH(a_name) kkc_arch_##a_name


/** Casts to the architecture object. */
#define KKC_ARCH_OBJ(o) ((kkc_arch_obj_t)o)

/** 
 * Declares a KKC architecture.  This is the preferred way of declaring
 * of a new architecture by specific architecture modules.
 * 
 *
 * @param a_name - name of the kkc architecture identifier (not quoted)
 * @param a_ops - pointer to the architecture specific operations
 * @param a_data - pointer to the architecture specific data
 */
#define KKC_ARCH_DECLARE(a_name, a_ops, a_data)	\
struct kkc_arch KKC_ARCH(a_name) = {		\
	.name = #a_name,			\
	.arch_ops = a_ops,			\
	.owner = THIS_MODULE,			\
	.data = a_data				\
};						\
EXPORT_SYMBOL_GPL(KKC_ARCH(a_name))


/** 
 * \<\<public\>\> Architecture name accessor.
 *
 * @param *self - pointer to this architecture instance
 * @return pointer to the architecture describing string
 */
static inline char* kkc_arch_name(struct kkc_arch *self)
{
	return self->name;
}

/** 
 * \<\<public\>\> Increments the reference counter on the module
 * owning the architecture. So that he module can't get unloaded when
 * being used.
 *
 * @param *self - pointer to this architecture instance
 * @return pointer to the architecture if succeeded in getting the module or NULL
 */
static inline struct kkc_arch* kkc_arch_get(struct kkc_arch *self)
{
       if (try_module_get(self->owner))
	       return self;
       return NULL;
}

/** 
 * \<\<public\>\> Retrieves an architecture by name.  This requires
 * merging the architecture name with the architecture prefix and
 * asking the kernel to retrieve the symbol specified by this string.
 *
 * This is a class method
 * @param arch_name - name of the architecture requested (e.g. 'tcp')
 * @return - instance of the kkc_arch that handles the specified type
 */
static inline struct kkc_arch* kkc_arch_get_by_name(char *arch_name)
{

	char sym[sizeof(KKC_ARCH_PREFIX_STR) + KKC_MAX_ARCH_LENGTH];
	strlcpy(sym, KKC_ARCH_PREFIX_STR, sizeof(KKC_ARCH_PREFIX_STR) + KKC_MAX_ARCH_LENGTH);
	strlcat(sym, arch_name, sizeof(KKC_ARCH_PREFIX_STR) + KKC_MAX_ARCH_LENGTH);
	mdbg(INFO3, "Getting architecture '%s', prefix: '%s'", sym, MODULE_SYMBOL_PREFIX);

	return (struct kkc_arch*) __symbol_get((const char*)sym);
}

/** 
 * \<\<public\>\> Decrements the reference counter on the module
 * owning the architecture.
 *
 * @param *self - pointer to this architecture instance
 */
static inline void kkc_arch_put(struct kkc_arch *self)
{
	return module_put(self->owner);
}

/** 
 * \<\<public\>\> Calls the constructor to build the architecture
 * specific object.
 *
 * @param *self - pointer to this architecture instance
 * @return architecture object instance or NULL
 */
static inline kkc_arch_obj_t kkc_arch_obj_new(struct kkc_arch *self)
{
	kkc_arch_obj_t obj = NULL;
	if (self->arch_ops && self->arch_ops->kkc_arch_obj_new)
		obj = self->arch_ops->kkc_arch_obj_new();
	return obj;
}
/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef KKC_ARCH_PRIVATE

#endif /* KKC_ARCH_PRIVATE */


/**
 * @}
 */


#endif /* _KKC_ARCH_H */


