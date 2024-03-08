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
#include "dofile.h"

jmp_buf savestate;
char *dirname[MAX_SIZE];

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
char *dptr;
char *aptr;
varval cmdargs;

/* get executable directory name from argv[0] */

memset(dirname,0,MAX_SIZE);

aptr=argv[0];
dptr=dirname;

while(aptr != strrchr(argv[0],'/')) {		/* from first character to last / in filename */
	*dptr++=*aptr++;
}

InitializeFunctions();						/* Initialize functions */

/* get command-line arguments */

CreateVariable("programname","STRING",1,1);
cmdargs.s=malloc(MAX_SIZE);

strcpy(cmdargs.s,argv[0]);
UpdateVariable("programname",NULL,&cmdargs,0,0);

CreateVariable("command","STRING",1,1);

cmdargs.s=malloc(MAX_SIZE);

memset(cmdargs.s,0,MAX_SIZE);

for(count=1;count<argc;count++) {
	strcat(cmdargs.s,argv[count]);
}

UpdateVariable("command",NULL,&cmdargs,0,0);

free(cmdargs.s);

/* intialize command-line arguments */

signal(SIGINT,signalhandler);		/* register signal handler */

if(argc == 1) {					/* no arguments */ 
	InteractiveMode();			/* run interpreter in interactive mode */
}
else
{
	ClearInteractiveModeFlag();

	ExecuteFile(argv[1]);						/* execute file */
}

FreeFunctions();

exit(0);
}

/*
 * Signal handler
 *
 * In: sig	Signal number
 *
 * Returns error number on error or 0 on success
 *
 */
void signalhandler(int sig) {

if(sig == SIGINT) {			/* ctrl-c */
	if(GetInteractiveModeFlag() == TRUE) {		/* if in interactive mode */
		if(GetIsRunningFlag() == TRUE) {		/* is running */
			printf("Program stopped. Type continue to resume\n");

			ClearIsRunningFlag();

			setjmp(savestate);		/* save program state */
	   	}
	   	else
	   	{
			printf("\nCtrl-C. Type QUIT to leave.\n");
	   	}
	}
	else
	{
			printf("Terminated by Ctrl-C\n");
			exit(0);
	}

   	return;
}

printf("Signal %d received\n",sig);
}

void GetExecutableDirectoryName(char *name) {
	strcpy(name,dirname);
}
	
