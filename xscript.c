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
#include "define.h"

/*
 * Main function
 *
 * In: int argc		Number of arguments
 *     char **argv	Arguments
 *
 * Returns: Nothing
 *
 */

extern char *TokenCharacters;

void signalhandler(int sig);
extern char *currentptr;
jmp_buf savestate;

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

 printf("XScript Version %d.%d\n\n",XSCRIPT_VERSION_MAJOR,XSCRIPT_VERSION_MINOR);

 InteractiveMode();
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
 switch(sig) {	
  case SIGINT:			/* ctrl-c */
   if(GetIsRunningFlag() == TRUE) {		/* is running */
	printf("Program suspended. Type continue to resume\n");
	SetIsRunningFlag(FALSE);

	setjmp(savestate);		/* save program state */
   }
   else
   {
	printf("\nCtrl-C. Type QUIT to leave.\n");
   }

   return;

  default:
   printf("Signal %d recieved\n",sig);
  return;
  }
}


