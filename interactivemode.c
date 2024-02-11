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

/* Interactive mode */

#include <stdio.h>
#include <setjmp.h>
#include "errors.h"
#include "size.h"
#include "interactivemode.h"
#include "variablesandfunctions.h"
#include "dofile.h"
#include "statements.h"
#include "version.h"

extern jmp_buf savestate;

void InteractiveMode(void) {
char *tokens[MAX_SIZE][MAX_SIZE];
char *endstatement[MAX_SIZE];
int block_statement_nest_count=0;
char *b;
char *linebuf[MAX_SIZE];
char *buffer;
char *bufptr;
int statementcount;
char *tokenchars[MAX_SIZE];
char *functionname[MAX_SIZE];
int tc;
char *blockstatement[MAX_SIZE];

SetInteractiveModeFlag();

SetCurrentFile("");				/* No file currently */

buffer=malloc(INTERACTIVE_BUFFER_SIZE);		/* allocate buffer for interactive mode */
if(buffer == NULL) {
	 perror("xscript");
	 exit(NO_MEM);
}

bufptr=buffer;

SetCurrentBufferPosition(buffer);		/* set buffer positoon to start */

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

	GetTokenCharacters(tokenchars);			/* get token characters */

	tc=TokenizeLine(bufptr,tokens,tokenchars);	/* tokenize line */

	/* remove newline */

	if(strlen(tokens[0]) > 1) {
	    b=tokens[0];
	    b += (strlen(tokens[0])-1);

	    if((*b == '\n') || (*b == '\r')) *b=0;
	}

	if(IsBlockStatement(tokens[0]) == TRUE) {	/* if block statement */
		strcpy(blockstatement,tokens[0]);

		block_statement_nest_count++;
	}

	if(IsEndStatementForStatement(blockstatement,tokens[0]) == TRUE) {	/* if at end of block statement */
		block_statement_nest_count--;

		bufptr=buffer;
	}
	 
	if(block_statement_nest_count == 0) {
		bufptr=buffer;

		GetCurrentFunctionName(functionname);		/* get current function name */

		do {
	   		if(check_breakpoint(GetCurrentFunctionLine(),functionname) == TRUE) {	/* breakpoint found */
				printf("Breakpoint in function %s on line %d reached\n",functionname,GetCurrentFunctionLine());
					ClearIsRunningFlag();

					setjmp(savestate);		/* save program state */

					break;
	   		}
	
			SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));

	   		ExecuteLine(bufptr);

	   		bufptr += strlen(bufptr);
	 } while(*bufptr != 0);

	   memset(buffer,0,INTERACTIVE_BUFFER_SIZE);

	   bufptr=buffer;
		
           SetCurrentBufferPosition(buffer);	   
	}
	else
	{
	  bufptr += strlen(bufptr);
	}

	} 
}

int quit_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	exit(0);
}

int continue_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if(GetIsRunningFlag() == FALSE) {
		printf("No program running\n");
	}
	else
	{
	printf("Continuing\n");

	SetIsRunningFlag();
	longjmp(savestate,0);

	}

}

int variables_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
list_variables(tokens[1]);
}

int load_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if(tc < 1) {						/* Not enough parameters */
	 PrintError(SYNTAX_ERROR);
	 return(SYNTAX_ERROR);
	}

	LoadFile(tokens[1]);
}

int run_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *currentfile[MAX_SIZE];


GetCurrentFile(currentfile);		/* get current file */

if(strlen(currentfile) == 0) {		/* check if there is a file */
	printf("No file loaded\n");
	return(0);
}

ExecuteFile(currentfile);
}

int single_step_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	int StepCount=0;
	int count;
	char *linebuf[MAX_SIZE];
	char *buf;

	if(strlen(tokens[1]) > 0) {
	SubstituteVariables(1,tc,tokens,tokens);

	StepCount=doexpr(tokens,1,tc);
	}
	else
	{
	StepCount=1;
	}

	for(count=0;count<StepCount;count++) {
		SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));

		buf=GetCurrentBufferPosition();

		if(*buf == 0) {
			printf("End reached\n");
			return(0);
		 }

	 	 ExecuteLine(linebuf);			/* run statement */
	}

}

int set_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if(tc < 2) {						/* Not enough parameters */
	 PrintError(SYNTAX_ERROR);
	 return(SYNTAX_ERROR);
	}

	if(strcmpi(tokens[1],"BREAKPOINT") == 0) {		/* set breakpoint */
	set_breakpoint(atoi(tokens[2]),tokens[3]);
	return(0);
	}

	printf("Invalid sub-command\n");
	return(0);
}

int clear_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if(tc < 2) {						/* Not enough parameters */
	 PrintError(SYNTAX_ERROR);
	 return(SYNTAX_ERROR);
	}

	if(strcmpi(tokens[1],"BREAKPOINT") == 0) {		/* set breakpoint */
	clear_breakpoint(atoi(tokens[2]),tokens[3]);
	return(0);
	}

	printf("Invalid sub-command\n");
	return(0);
}

