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

int main(int argc, char **argv) {
int count;
char *args[MAX_SIZE];
char *filename=NULL;
char *buffer;
char *bufptr;
varval cmdargs;
char *tokens[MAX_SIZE][MAX_SIZE];
char *endstatement[MAX_SIZE];
int block_statement_nest_count=0;
char *b;

InitializeFunctions();						/* Initialize functions */

/* intialize command-line arguments */

CreateVariable("argc",VAR_INTEGER,argc,0);

if(argc == 1) {					/* no arguments */ 
 signal(SIGINT,signalhandler);		/* register signal handler */

/* add argv[0] = executable and argc=1 */

 CreateVariable("argv",VAR_STRING,1,0);			/* add command line arguments variable */

 strcpy(cmdargs.s,argv[0]);
 UpdateVariable("argv",&cmdargs,0,0);

 buffer=malloc(INTERACTIVE_BUFFER_SIZE);		/* allocate buffer for interactive mode */
 if(buffer == NULL) {
  perror("xscript");
  exit(NO_MEM);
 }

 bufptr=buffer;

 printf("XScript Version %d.%d\n\n",XSCRIPT_VERSION_MAJOR,XSCRIPT_VERSION_MINOR);

 while(1) {
  if(block_statement_nest_count == 0) {
	printf(">");
  }
  else
  {
	printf("...");
  }

  fgets(bufptr,MAX_SIZE,stdin);			/* read line */

  TokenizeLine(bufptr,tokens,TokenCharacters);			/* tokenize line */

/* remove newline */

   if(strlen(tokens[0]) > 1) {
     b=tokens[0];
     b += (strlen(tokens[0])-1);
     if((*b == '\n') || (*b == '\r')) *b=0;
   }

  touppercase(tokens[0]);

  if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"WHILE") == 0) || (strcmpi(tokens[0],"FOR") == 0) || (strcmpi(tokens[0],"FUNCTION") == 0)) {
   sprintf(endstatement,"END%s",tokens[0]);
   
   block_statement_nest_count++;
  }
 
  if(strcmp(tokens[0],endstatement) == 0) {
    block_statement_nest_count--;

    bufptr=buffer;
  }
  
  if(block_statement_nest_count == 0) {
   printf("%s",buffer);

   bufptr=buffer;

   do {
  
    ExecuteLine(bufptr);			/* execute line */

    bufptr += strlen(bufptr);
  } while(*bufptr != 0);

    memset(buffer,0,INTERACTIVE_BUFFER_SIZE);

    bufptr=buffer;
  }
  else
  {
   bufptr += strlen(bufptr);
  }

 }

}
else
{
 cmdargs.i=argc;
 UpdateVariable("argc",&cmdargs,count,0);
 
 memset(cmdargs.s,0,MAX_SIZE);

 for(count=0;count<argc;count++) {
  strcpy(cmdargs.s,argv[count]);
  UpdateVariable("argv",&cmdargs,count,0);
 }

 ExecuteFile(argv[1]);						/* execute file */
}

FreeFunctions();

exit(0);
}


void signalhandler(int sig) {
 if(sig == SIGTRAP) {
   printf("Keyboard break. Type END to quit.\n");
   return;
 }
 else {
   printf("Signal caught");
   return;
  }

}

