/**
 * @file tcmi_rpc.h - .
 *                      
 * 
 *
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_rpc.h,v 1.5 2007/10/20 14:24:20 stavam2 Exp $
 *
 * This file is part of Task Checkpointing and Migration Infrastructure(TCMI)
 * Copyleft (C) 2007  Petr Malat
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

#ifndef _TCMI_RPC_H
#define _TCMI_RPC_H

#include "tcmi_rpc_numbers.h"
/** @defgroup tcmi_rpc_class tcmi_rpc class 
 *
 * @ingroup tcmi_syscall_group
 *
 * A \<\<virtual\>\> RPC class, used as parent for both guest and shadow side.
 *
 * This class is used for calling methods on shadow on guest request. If guest task calls RPC,
 * the remote side is executed by coresponding shadow task. Communication is done using 
 * \link tcmi_rpc_procmsg tcmi_rpc_procmsg class \endlink for RPC request and 
 * \link tcmi_rpcresp_procmsg tcmi_rpcresp_procmsg class \endlink for response.
 * 
 * @{
 */

/** Max remote procedure call parameters */
#define TCMI_RPC_MAXPARAMS 6

struct tcmi_rpc;

/** \<\<public\>\> Universal procedure to do RPC */
long tcmi_rpc_do(struct tcmi_rpc *self, unsigned int rpc_num, long parameters[TCMI_RPC_MAXPARAMS]);

/** \<\<public\>\> Do a rpc without parameters. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call0(struct tcmi_rpc *self, unsigned int rpc_num)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/** \<\<public\>\> Do a rpc with 1 parameter. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param param1  - First parametr of called procedure
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call1(struct tcmi_rpc *self, unsigned int rpc_num, long param1)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	parameters[0] = param1;
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/** \<\<public\>\> Do a rpc with 2 parameters. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param param1  - First parametr of called procedure
 * @param param2  - Second parametr of called procedure
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call2(struct tcmi_rpc *self, unsigned int rpc_num, long param1, long param2)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	parameters[0] = param1;
	parameters[1] = param2;
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/** \<\<public\>\> Do a rpc with 3 parameters. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param param1  - First parametr of called procedure
 * @param param2  - Second parametr of called procedure
 * @param param3  - Third parametr of called procedure
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call3(struct tcmi_rpc *self, unsigned int rpc_num, long param1, long param2, long param3)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	parameters[0] = param1;
	parameters[1] = param2;
	parameters[2] = param3;
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/** \<\<public\>\> Do a rpc with 4 parameters. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param param1  - First parametr of called procedure
 * @param param2  - Second parametr of called procedure
 * @param param3  - Third parametr of called procedure
 * @param param4  - 4th parametr of called procedure
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call4(struct tcmi_rpc *self, unsigned int rpc_num, long param1, long param2, long param3, long param4)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	parameters[0] = param1;
	parameters[1] = param2;
	parameters[2] = param3;
	parameters[3] = param4;
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/** \<\<public\>\> Do a rpc with 4 parameters. 
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param param1  - First parametr of called procedure
 * @param param2  - Second parametr of called procedure
 * @param param3  - Third parametr of called procedure
 * @param param4  - 4th parametr of called procedure
 * .....
 *
 * @return RPC return code
 * */
static inline long tcmi_rpc_call6(struct tcmi_rpc *self, unsigned int rpc_num, long param1, long param2, long param3, long param4, long param5, long param6)
{
	long parameters[TCMI_RPC_MAXPARAMS] = {0, 0, 0, 0, 0, 0};
	parameters[0] = param1;
	parameters[1] = param2;
	parameters[2] = param3;
	parameters[3] = param4;
	parameters[4] = param5;
	parameters[5] = param6;
	return tcmi_rpc_do(self, rpc_num, parameters); 
}

/********************** PRIVATE METHODS AND DATA ******************************/
#ifdef TCMI_RPC_PRIVATE
/** \<\<private\>\> Definition of tcmi_rpc class */
struct tcmi_rpc
{
	/** Call table */
	long (**calltable)(unsigned, long[TCMI_RPC_MAXPARAMS]);
};
#endif /* TCMI_RPC_PRIVATE */
/**
 * @}
 */

#endif /* _TCMI_RPC_H */

