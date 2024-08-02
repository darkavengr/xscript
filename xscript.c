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
int savestatereturn;
char *dirname[MAX_SIZE];
bool IsPaused=FALSE;

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
int returnvalue;
struct sigaction signalaction;

/* get executable directory name from argv[0] */

memset(dirname,0,MAX_SIZE);

aptr=argv[0];
dptr=dirname;

while(aptr != strrchr(argv[0],'/')) {		/* from first character to last / in filename */
	*dptr++=*aptr++;
}

memset(args,0,MAX_SIZE);

/* get arguments */

for(count=2;count<argc;count++) {
	strcat(args,argv[count]);

	if(count < argc-1) strcat(args," ");
}

InitializeMainFunction(args);			/* Initialize main function */

signalaction.sa_sigaction=&signalhandler;
signalaction.sa_flags=SA_NODEFER;

if(sigaction(SIGINT,&signalaction,NULL) == -1) {		/* set signal handler */
	perror("xscript:");
	exit(1);
}

/* intialize command-line arguments */

if(argc == 1) {					/* no arguments */ 
	InteractiveMode();			/* run interpreter in interactive mode */
}
else
{
	ClearInteractiveModeFlag();

	returnvalue=ExecuteFile(argv[1]);	/* execute file */
	if(returnvalue) {
		SetLastError(returnvalue);

		PrintError(returnvalue);

		FreeFunctions();
		exit(returnvalue);
	}
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
void signalhandler(int sig,siginfo_t *info,void *ucontext) {

if(sig == SIGINT) {			/* ctrl-c */

	if(GetInteractiveModeFlag() == TRUE) {		/* if in interactive mode */
		if(GetIsRunningFlag() == TRUE) {		/* is running */
			printf("Program stopped. Type continue to resume\n");

			ClearIsRunningFlag();

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

