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
extern int savestatereturn;

char *InteractiveModeBuffer=NULL;
char *InteractiveModeBufferPosition=NULL;

/*
 * Run XScript in interactive mode
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */
void InteractiveMode(void) {
char *tokens[MAX_SIZE][MAX_SIZE];
char *endstatement[MAX_SIZE];
int block_statement_nest_count=0;
char *b;
char *linebuf[MAX_SIZE];
int statementcount;
char *tokenchars[MAX_SIZE];
char *functionname[MAX_SIZE];
int tc;
BLOCKSTATEMENTSAVE *blockstatementsave_head=NULL;
BLOCKSTATEMENTSAVE *blockstatementsave_end=NULL;
BLOCKSTATEMENTSAVE *savelast;
char *SaveBufferAddress;
size_t returnvalue;

SetInteractiveModeFlag();
ClearIsRunningFlag();

SetCurrentFile("");				/* No file currently */

InteractiveModeBuffer=malloc(INTERACTIVE_BUFFER_SIZE);		/* allocate buffer for interactive mode */
if(InteractiveModeBuffer == NULL) {
	 perror("xscript");
	 exit(NO_MEM);
}

InteractiveModeBufferPosition=InteractiveModeBuffer;		/* set buffer position to start */


printf("XScript Version %d.%d\n\n",XSCRIPT_VERSION_MAJOR,XSCRIPT_VERSION_MINOR);

while(1) {	
	SwitchToInteractiveModeBuffer();		/* switch to interactive mode buffer */

	if(block_statement_nest_count == 0) {
		printf(">");
	}
	else
	{
		printf("...");
	}

	fgets(InteractiveModeBufferPosition,MAX_SIZE,stdin);			/* read line */

	GetTokenCharacters(tokenchars);			/* get token characters */

	tc=TokenizeLine(InteractiveModeBufferPosition,tokens,tokenchars);	/* tokenize line */

	/* remove newline */

	if(strlen(tokens[0]) > 1) {
	    b=tokens[0];
	    b += (strlen(tokens[0])-1);

	    if((*b == '\n') || (*b == '\r')) *b=0;
	}

	if(IsBlockStatement(tokens[0]) == TRUE) {	/* if block statement */

		/* save start of block statement in linked list */

		if(blockstatementsave_head == NULL) {	/* first */
			blockstatementsave_head=malloc(sizeof(BLOCKSTATEMENTSAVE));

			if(blockstatementsave_head == NULL) {
				PrintError(NO_MEM);
				continue;
			}

			blockstatementsave_end=blockstatementsave_head;
			blockstatementsave_end->last=blockstatementsave_head;
			blockstatementsave_end->next=NULL;
		}
		else
		{
			blockstatementsave_end->next=malloc(sizeof(BLOCKSTATEMENTSAVE));
			if(blockstatementsave_end->next == NULL) {
				PrintError(NO_MEM);
				continue;
			}

			savelast=blockstatementsave_end->last;
			blockstatementsave_end=blockstatementsave_end->next;
			blockstatementsave_end->last=savelast;
		}

		strcpy(blockstatementsave_end->token,tokens[0]);
		block_statement_nest_count++;
	}

	if(blockstatementsave_end != NULL) {
		if(IsEndStatementForStatement(blockstatementsave_end->token,tokens[0]) == TRUE) {	/* if at end of block statement */
			block_statement_nest_count--;
			blockstatementsave_end=blockstatementsave_end->last;	/* point to previous in list */

			InteractiveModeBufferPosition=InteractiveModeBuffer;	/* point to start */
		}
	}

	if(block_statement_nest_count == 0) {			/* if at end of entering statements */
		SwitchToInteractiveModeBuffer();		/* switch to interactive mode buffer */

		GetCurrentFunctionName(functionname);		/* get current function name */

		SetIsRunningFlag();

		do {
	   		if(check_breakpoint(GetCurrentFunctionLine(),functionname) == TRUE) {	/* breakpoint found */
				printf("Breakpoint in function %s on line %d reached\n",functionname,GetCurrentFunctionLine());
				ClearIsRunningFlag();

				setjmp(savestate);		/* save program state */

				break;
	   		}

			if(GetIsRunningFlag() == FALSE)	break;		/* not running */

			SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));	/* read line from buffer */
		
			returnvalue=ExecuteLine(linebuf);		/* run line */
	   		if(returnvalue != 0) {
				SetLastError(returnvalue);

				PrintError(returnvalue);
				break;
			}
				
			InteractiveModeBufferPosition=GetCurrentBufferPosition();
			InteractiveModeBufferPosition += strlen(InteractiveModeBufferPosition);	/* point to next statement */
			SetCurrentBufferPosition(InteractiveModeBufferPosition);

	 	} while(*InteractiveModeBufferPosition != 0);

		SetCurrentBufferPosition(SaveBufferAddress);	/* restore buffer address */

	   	ClearIsRunningFlag();

	   	memset(InteractiveModeBuffer,0,INTERACTIVE_BUFFER_SIZE);

	   	InteractiveModeBufferPosition=InteractiveModeBuffer;		/* set buffer positoon to start */
	}
	else
	{
		InteractiveModeBufferPosition += strlen(InteractiveModeBufferPosition);	/* point to next statement */
	}	
	 }
}

/*
 * Quit statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int quit_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
exit(0);
}

/*
 * Continue statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int continue_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
ExecuteFile(NULL);
}

/*
 * Variables command
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int variables_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
list_variables(tokens[1]);
}

/*
 * Load statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int load_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;
char *filename[MAX_SIZE];

if(tc < 1) {						/* Not enough parameters */
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

if(IsValidString(tokens[1]) == FALSE) {			/* is valid string */
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

StripQuotesFromString(tokens[1],filename);
	
return(LoadFile(filename));
}

/*
 * Run statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int run_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *currentfile[MAX_SIZE];

GetCurrentFile(currentfile);		/* get current file */

if(strlen(currentfile) == 0) {		/* check if there is a file */
	printf("No file loaded\n");
	return(0);
}

ExecuteFile(currentfile);
}

/*
 * Trace statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int trace_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(tc < 2) {			/* Display trace status */
	if(GetTraceFlag() == TRUE) {
		printf("Trace is ON\n");
	}
	else
	{
		printf("Trace is OFF\n");
	}

	return;		
}

if(strcmpi(tokens[1],"ON") == 0) {		/* enable trace */
	SetTraceFlag();
}
else if(strcmpi(tokens[1],"OFF") == 0) {		/* disable trace */
	ClearTraceFlag();
}
else
{
	PrintError(INVALID_VALUE);
}

return(0);
}

/*
 * Set breakpoint
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
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
return(INVALID_VALUE);
}

/*
 * Clear breakpoint
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
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

void SwitchToInteractiveModeBuffer(void) {
SetCurrentBufferPosition(InteractiveModeBuffer);
}

int stacktrace_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
FUNCTIONCALLSTACK *stack;
int callstackcount=0;
	
stack=GetFunctionCallStackTop();

while(stack != NULL) {
	printf("#%d	%s\n",callstackcount++,stack->name);

	stack=stack->last;
}

return(0);
}

