/** @file dbg.h - a debugging and logging framework, based on mdbg
 * library by Martin Kacer.  The standard message types are divided
 * into four categories: 
 * -# info messages (tracing only) 
 * -# warnings (something strange happend) 
 * -# errors (it is not possible to do something which should be possible to do)
 * -# critical errors (serious program failures). 
 *
 * Other than that, there is a number from 1 (very important) to 4
 * (least important) assigned to each of the messages - the severity.
 * Whenever the logging macro mdbg() is called, the given type of the
 * message is compared to the mask stored in MDBG_MASK. All the bits
 * that are set in the message type must also be set in the defined
 * mask. In other words, the condition "(~mask & type) == 0" must be
 * satisfied.  The standard messages with higher severity have always
 * less bits set than the ones with lower severity. That means
 * enabling some type of message enables also all the other types in
 * the same category which have higher severity. E.g. enabling
 * 'MDBG_INFO2' enables also 'MDBG_INFO1' too.
 *
 * There are two macros provided:
 * - mdbg()
 * - minfo() - standard info output by the kernel application. The message 
 * is stored into a corresponding log file set by kernel logging framework.
 * The user still specifies a message level type. This is needed as the message
 * is still passed onto mdbg().
 *
 * The user has to define selected levels for each
 * category if he/she doesn't want to use the defaults (which is
 * 4). For example in a Makefile, it can be done as follows:
 * \verbatim
 * EXTRA_CFLAGS += -DMDBG_CRIT=4 -DMDBG_ERR=4 -DMDBG_WARN=4 -DMDBG_INFO=4
 * \endverbatim
 * This ensures that levels 1-4 will be displayed in the debug log for 
 * each category.
 *
 * Date: 05/05/2005
 *
 * Author: Jan Capek, Martin Kacer
 *
 * $Id: dbg.h,v 1.6 2008/05/02 19:59:20 stavam2 Exp $
 *
 * This file is part of Clondike project
 * Copyleft (C) 2005  Jan Capek, Martin Kacer
 * 
 * Clondike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Clondike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DBG_H
#define DBG_H

#include <linux/kernel.h>
#include <linux/sched.h>
#include <clondike/tcmi/tcmi_dbg.h>

/*********************************************/
/***  Message types  as used in the source ***/
/*********************************************/
#define MDBG_CRIT1   0x00001000
#define MDBG_CRIT2   0x00003000
#define MDBG_CRIT3   0x00007000
#define MDBG_CRIT4   0x0000F000
#define MDBG_ERR1    0x00000100
#define MDBG_ERR2    0x00000300
#define MDBG_ERR3    0x00000700
#define MDBG_ERR4    0x00000F00
#define MDBG_WARN1   0x00000010
#define MDBG_WARN2   0x00000030
#define MDBG_WARN3   0x00000070
#define MDBG_WARN4   0x000000F0
#define MDBG_INFO1   0x00000001
#define MDBG_INFO2   0x00000003
#define MDBG_INFO3   0x00000007
#define MDBG_INFO4   0x0000000F

/* Default levels for each category */
#ifndef MDBG_CRIT 
#error MDBG_CRIT not defined
#endif
#ifndef MDBG_ERR 
#error MDBG_ERR not defined
#endif
#ifndef MDBG_WARN 
#error MDBG_WARN not defined
#endif
#ifndef MDBG_INFO 
#error MDBG_INFO not defined
#endif

/* Description of each message */
#define MDBG_DESC_CRIT(level) "<crit"#level">"
#define MDBG_DESC_ERR(level)  "<err"#level"> "
#define MDBG_DESC_WARN(level) "<warn"#level">"
#define MDBG_DESC_INFO(level) "<info"#level">"

/* This assigns each message a description */
#define MDBG_DESC_CRIT4  MDBG_DESC_CRIT(4)
#define MDBG_DESC_CRIT3  MDBG_DESC_CRIT(3)
#define MDBG_DESC_CRIT2  MDBG_DESC_CRIT(2)
#define MDBG_DESC_CRIT1  MDBG_DESC_CRIT(1)
#define MDBG_DESC_ERR4   MDBG_DESC_ERR(4) 
#define MDBG_DESC_ERR3   MDBG_DESC_ERR(3)
#define MDBG_DESC_ERR2   MDBG_DESC_ERR(2)
#define MDBG_DESC_ERR1   MDBG_DESC_ERR(1)
#define MDBG_DESC_WARN4  MDBG_DESC_WARN(4)
#define MDBG_DESC_WARN3  MDBG_DESC_WARN(3)
#define MDBG_DESC_WARN2  MDBG_DESC_WARN(2)
#define MDBG_DESC_WARN1  MDBG_DESC_WARN(1)
#define MDBG_DESC_INFO4  MDBG_DESC_INFO(4)
#define MDBG_DESC_INFO3  MDBG_DESC_INFO(3)
#define MDBG_DESC_INFO2  MDBG_DESC_INFO(2)
#define MDBG_DESC_INFO1  MDBG_DESC_INFO(1)

/* This assigns each message a valid kernel log level - used by
 * minfo(). All debug messages are assigned the KERN_DEBUG level by
 * default */
#define MDBG_KERN_CRIT4  KERN_CRIT
#define MDBG_KERN_CRIT3  KERN_CRIT
#define MDBG_KERN_CRIT2  KERN_CRIT 
#define MDBG_KERN_CRIT1  KERN_CRIT 
#define MDBG_KERN_ERR4   KERN_ERR
#define MDBG_KERN_ERR3   KERN_ERR
#define MDBG_KERN_ERR2   KERN_ERR   
#define MDBG_KERN_ERR1   KERN_ERR 
#define MDBG_KERN_WARN4  KERN_WARNING
#define MDBG_KERN_WARN3  KERN_WARNING
#define MDBG_KERN_WARN2  KERN_WARNING
#define MDBG_KERN_WARN1  KERN_WARNING
#define MDBG_KERN_INFO4  KERN_INFO
#define MDBG_KERN_INFO3  KERN_INFO
#define MDBG_KERN_INFO2  KERN_INFO
#define MDBG_KERN_INFO1  KERN_INFO

/* creates a mask that has first b bits starting from LSb set and the
 * whole mask is then shifted by s bits to the right */
#define MDBG_SET_BITS(b, s) (((1 << b) - 1) << s)

/* Creates the debug level mask from critical, error, warning, info
 * levels */
#define MDBG_MAKE_MASK(C, E, W, I)		\
 (MDBG_SET_BITS(C, 12) | MDBG_SET_BITS(E, 8) |	\
  MDBG_SET_BITS(W, 4)  | MDBG_SET_BITS(I, 0))

/* Extend the define application name with : */
#ifndef APP_NAME
#error APP_NAME not defined
#endif

#define CLONDIKE_STR(s) #s
#define MAKE_APP_NAME(name) CLONDIKE_STR(name)
#define __APP_NAME MAKE_APP_NAME(APP_NAME)

#ifndef MDBG_MASK
#define MDBG_MASK \
 MDBG_MAKE_MASK(MDBG_CRIT, MDBG_ERR, MDBG_WARN, MDBG_INFO)
#endif

#ifdef CONFIG_TCMI_DEBUG
#define mdbg(type, fmt, args...)								\
do {												\
	if ( ((MDBG_##type & MDBG_MASK) == MDBG_##type) /*&&  tcmi_dbg */ )						\
		printk(KERN_DEBUG __APP_NAME MDBG_DESC_##type " %s()[%d]:" fmt "\n", __FUNCTION__, current->pid, ## args);\
} while(0)

/* Generic information macro, in addition produces a debug log */
/* 	printk(MDBG_KERN_##type __APP_NAME MDBG_DESC_##type ":" fmt "\n", ## args);	*/
#define minfo(type, fmt, args...)							\
do {											\
	if ( ((MDBG_##type & MDBG_MASK) == MDBG_##type) )				\
	  	printk(MDBG_KERN_##type __APP_NAME MDBG_DESC_##type ":" fmt "\n", ## args);\
} while(0)
#define debug_enabled (tcmi_dbg)
#else // CONFIG_TCMI_DEBUG
#define mdbg(type, fmt, args...) do {} while (0)
#define minfo(type, fmt, args...) do {} while (0)
#define debug_enabled 0
#endif


#endif /* DBG_H */
