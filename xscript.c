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
#include "errors.h"

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
varval cmd;
varval args;
varval version;
int returnvalue;
struct sigaction signalaction;
struct sigaction oldsignalaction;
sigset_t sigset;
char *temp[MAX_SIZE];
struct rlimit new_rlimit;

/* set stack size */
if(getrlimit(RLIMIT_STACK,&new_rlimit) == -1) {
	perror("xscript:");
	exit(1);
}

new_rlimit.rlim_cur=(unsigned long) 1 << (sizeof(size_t)-1)*8;
new_rlimit.rlim_max=(unsigned long) 1 << (sizeof(size_t)-1)*8;

if(setrlimit(RLIMIT_STACK,&new_rlimit) == -1) {
	perror("xscript:");
	exit(1);
}

InitalizeModules();		/* initialize modules */

/****
 Set global constants
*/

cmd.s=calloc(1,MAX_SIZE);		/* allocate args buffer */
if(cmd.s == NULL) {
	PrintError(NO_MEM);
	return(-1);
}

args.s=calloc(1,MAX_SIZE);		/* allocate args buffer */
if(args.s == NULL) {
	PrintError(NO_MEM);
	return(-1);
}

cmd.type=VAR_STRING;
args.type=VAR_STRING;	/* set type */

/* PROGRAMNAME */

if(argc > 1) {
	if(*argv[1] != '/') {			/* if it's a relative path */
		snprintf(cmd.s,MAX_SIZE,"\"%s/%s\"",getcwd(temp,MAX_SIZE),argv[1]);	/* prepend current directory to relative path */
	}
	else
	{
		snprintf(cmd.s,MAX_SIZE,"\"%s\"",argv[1]);	/* if is absolute path then just copy it */
	}
}

if(DefineGlobalConstant("PROGRAMNAME",&cmd) == -1) {		/* define global constant program name */
	PrintError(GetLastError());

	exit(GetLastError());
}

StripQuotesFromString(cmd.s,temp);		/* remove quotes from filename */

/* COMMAND */

if(argc >= 3) {
	for(count=2;count < argc;count++) {
		strncat(args.s,argv[count],MAX_SIZE);

		if(count < argc-1) strncat(args.s," ",MAX_SIZE);
	}
}

if(DefineGlobalConstant("COMMAND",&args) == -1) {		/* define global constant command */
	PrintError(GetLastError());
	exit(GetLastError());
}

/* VERSION */

version.type=VAR_NUMBER;		/* set type */
version.d=(double) XSCRIPT_VERSION_MAJOR + ((double) XSCRIPT_VERSION_MINOR/100) + ((double) XSCRIPT_VERSION_REVISION/1000);

if(DefineGlobalConstant("VERSION",&version) == -1) {		/* define global constant version */
	PrintError(GetLastError());
	exit(GetLastError());
}

/* set signal handler */
signalaction.sa_sigaction=&signalhandler;
signalaction.sa_flags=SA_NODEFER;
signalaction.sa_mask=sigset;

if(sigaction(SIGINT,&signalaction,&oldsignalaction) == -1) {
	perror("xscript:");
	exit(1);
}

if(argc == 1) {					/* no arguments */ 
	InteractiveMode();			/* run interpreter in interactive mode */
}
else
{
	ClearInteractiveModeFlag();

	if(ExecuteFile(temp,args.s) == -1) {	/* execute file */
		PrintError(GetLastError());

		free(cmd.s);
		free(args.s);

		cleanup();		/* deallocate lists */
		exit(returnvalue);
	}
}

free(cmd.s);
free(args.s);

cleanup();			/* deallocate lists */
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
			stop();
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

void cleanup(void) {
FreeFunctionsAndVariables();			/* free functions and variables */
FreeModulesList();				/* free modules */
}

