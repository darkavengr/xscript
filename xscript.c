/*  XScript Version 0.0.1
    (C) Matthew Boote 2020

    This file is part of XScript.

    XScript is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    XScript is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with XScript.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include "size.h"
#include "variablesandfunctions.h"
#include "xscript.h"
#include "modeflags.h"
#include "version.h"
#include "interactivemode.h"

jmp_buf savestate;

/*
 * Main function
 *
 * In: argc	Number of arguments
 *     argv	Arguments
 *
 * Returns: Nothing
 *
 */

int main(int argc, char **argv) {
int count;
char *args[MAX_SIZE];
char *filename=NULL;
varval cmdargs;

InitializeFunctions();						/* Initialize functions */

/* intialize command-line arguments */

CreateVariable("argc","INTEGER",0,0);

if(argc == 1) {					/* no arguments */ 
	signal(SIGINT,signalhandler);		/* register signal handler */

	/* add argv[0] = executable and argc=1 */

	CreateVariable("argv","STRING",1,0);			/* add command line arguments variable */

	cmdargs.s=malloc(strlen(argv[0]));				/* allocate string */
	if(cmdargs.s == NULL) return(-1);

	strcpy(cmdargs.s,argv[0]);

	UpdateVariable("argv",NULL,&cmdargs,0,0);

	cmdargs.i=1;
	UpdateVariable("argc",NULL,&cmdargs,0,0);

	InteractiveMode();			/* run interpreter in interactive mode */
}
else
{
	cmdargs.i=argc;

	UpdateVariable("argc",NULL,&cmdargs,count,0);

	for(count=0;count<argc;count++) {
		cmdargs.s=malloc(sizeof(argv[count]));

		strcpy(cmdargs.s,argv[count]);
		UpdateVariable("argv",NULL,&cmdargs,count,0);

		free(cmdargs.s);

	}

	ExecuteFile(argv[1]);						/* execute file */
}

FreeFunctions();

exit(0);
}

void signalhandler(int sig) {

if(sig == SIGINT) {			/* ctrl-c */
	if(GetIsRunningFlag() == TRUE) {		/* is running */
		printf("Program suspended. Type continue to resume\n");
		SetIsRunningFlag();

		setjmp(savestate);		/* save program state */
   	}
   	else
   	{
		printf("\nCtrl-C. Type QUIT to leave.\n");
   	}

   	return;
}

printf("Signal %d received\n",sig);
}

