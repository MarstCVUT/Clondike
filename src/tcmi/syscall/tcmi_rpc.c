/**
 * @file tcmi_rpc.c - .
 *                      
 * 
 *
 *
 * Date: 7/05/2007
 *
 * Author: Petr Malat
 *
 * $Id: tcmi_rpc.c,v 1.2 2007/09/02 12:09:55 malatp1 Exp $
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

#include <dbg.h>
#include <linux/errno.h>

#define TCMI_RPC_PRIVATE
#include "tcmi_rpc.h"

/** \<\<public\>\> Universal procedure to do RPC
 * @param *self - pointer to this tcmi_rpc class instance
 * @param rpc_num - Identification number of called procedure
 * @param parameters[] - Array of RPCs parameters
 *
 * @return RPC return code
 * */
long tcmi_rpc_do(struct tcmi_rpc *self, unsigned int rpc_num, long parameters[TCMI_RPC_MAXPARAMS])
{
	long ret = -EPERM;
	if( rpc_num > TCMI_MAX_RPC_NUM ){
		mdbg(ERR3, "%d isn't number of procedure", rpc_num);
	}
	else if( self->calltable[rpc_num] != NULL){
		mdbg(INFO2, "Calling RPC#%d(%lx, %lx, %lx, %lx, %lx) at %p", rpc_num,
				parameters[0], parameters[1], parameters[2],
				parameters[3], parameters[4], self->calltable[rpc_num]);
		ret = self->calltable[rpc_num](rpc_num, parameters);
		mdbg(INFO2, "RPC#%d returned %ld(%lx)", rpc_num, ret, ret);
	}
	else{
		mdbg(ERR2, "No handler for RPC#%d", rpc_num);
	}
	return ret;
}
