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
#include <sys/resource.h>

#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"
#include "xscript.h"
#include "modeflags.h"
#include "version.h"
#include "interactivemode.h"
#include "dofile.h"
#include "debugmacro.h"

sigjmp_buf savestate;
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
char *fullpath[MAX_SIZE];
struct rlimit new_rlimit;

/* set stack size */
if(getrlimit(RLIMIT_STACK,&new_rlimit) == -1) {
	perror("xscript:");
	exit(1);
}

new_rlimit.rlim_max=1024*1024*1024;

if(setrlimit(RLIMIT_STACK,&new_rlimit) == -1) {
	perror("xscript:");
	exit(1);
}

InitalizeModules();		/* initalize modules */

/* get executable directory name from argv[0] */

memset(dirname,0,MAX_SIZE);

aptr=argv[0];
dptr=dirname;

while(aptr != strrchr(argv[0],'/')) {		/* from first character to last / in filename */
	*dptr++=*aptr++;
}

/* get script's absolute path */

memset(fullpath,0,MAX_SIZE);

if(argc > 1) {
	if(*argv[1] != '/') {			/* if is relative path */
		sprintf(fullpath,"%s/%s",getcwd(args,MAX_SIZE),argv[1]);	/* prepend current directory to relative path */
	}
	else
	{
		strncpy(fullpath,argv[1],MAX_SIZE);	/* if is absolute path then just copy it */
	}
}

/* get arguments */

for(count=2;count<argc;count++) {
	strncat(args,argv[count],MAX_SIZE);

	if(count < argc-1) strncat(args," ",MAX_SIZE);
}

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

	if(ExecuteFile(fullpath,args) == -1) {	/* execute file */
		PrintError(GetLastError());

		cleanup();		/* deallocate lists */
		exit(returnvalue);
	}
}

cleanup();			/* deallocate lists */

asm("int $3");
exit(0);
}

/*
 * Signal handler
 *
 * In: sig	Signal number
 *
 * Returns nothing
 *
 */
void signalhandler(int sig,siginfo_t *info,void *ucontext) {

if(sig == SIGINT) {			/* ctrl-c */

	if(GetInteractiveModeFlag() == TRUE) {		/* if in interactive mode */
		if(GetIsRunningFlag() == TRUE) {		/* is running */
			printf("Program stopped. Type continue to resume\n");

			ClearIsRunningFlag();
			return;
	   	}
	   	else
	   	{
			printf("\nCtrl-C. Type QUIT to exit.\n");
	   	}
	}
	else
	{
			printf("xscript: Terminated by Ctrl-C\n");

			cleanup();		/* deallocate lists */
			exit(0);
	}

   	return;
}

printf("xscript: Signal %d received\n",sig);
}

void GetExecutableDirectoryName(char *name) {
strncpy(name,dirname,MAX_SIZE);
}

void cleanup(void) {
FreeFunctionsAndVariables();			/* free functions and variables */
FreeModulesList();				/* free modules */

FreeInteractiveModeBuffer();	/* free interactive mode buffer */
}

