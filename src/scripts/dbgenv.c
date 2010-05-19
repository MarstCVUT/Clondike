/**
 * @file dbgenv.c - sets debug flags according to environment
 *                      
 * Author: Petr Malat
 *
 * This file is part of Clondike.
 *
 * Clondike is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License version 2 as published by 
 * the Free Software Foundation.
 *
 * Clondike is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * Clondike. If not, see http://www.gnu.org/licenses/.
 */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	char buf[64], *env, *appname;
	const char * const mdbgvars[] = {
		"CRIT", "ERR", "WARN", "INFO"
	};
	unsigned i;

	if (argc != 2) return -1;

	appname = basename(dirname(argv[1]));
	printf("-DAPP_NAME=clondike-%s ", appname);

	for (i = 0; i < sizeof mdbgvars/sizeof mdbgvars[0]; i++) {
		sprintf(buf, "MDBG_%s_%s", appname, mdbgvars[i]);
		env = getenv(buf);
		if (env) printf("-DMDBG_%s=%s ", mdbgvars[i], env);
	}

	return 0;
}
