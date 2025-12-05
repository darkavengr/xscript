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
#include "module.h"
#include "variablesandfunctions.h"
#include "dofile.h"
#include "statements.h"
#include "version.h"
#include "debugmacro.h"

extern sigjmp_buf savestate;
extern int savestatereturn;
extern char *TokenCharacters;
		
jmp_buf single_step_save_state;
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
char *bufptr;
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
char *FunctionBufferAddress=NULL;
char *FunctionBufferPtr=NULL;
int FunctionBufferSize=0;
bool IsDeclaringFunction=FALSE;
int OldFunctionBufferSize=0;

SetInteractiveModeFlag();
ClearIsRunningFlag();

SetCurrentFile("");				/* No file currently */

InteractiveModeBuffer=malloc(INTERACTIVE_BUFFER_SIZE);		/* allocate buffer for interactive mode */
if(InteractiveModeBuffer == NULL) {
	 perror("xscript");
	 exit(NO_MEM);
}

InteractiveModeBufferPosition=InteractiveModeBuffer;		/* set buffer position to start */
SwitchToInteractiveModeBuffer();		/* switch to interactive mode buffer */

InitializeMainFunction(NULL,NULL);			/* Initialize main function */

printf("XScript version %d.%d.%d (C) Matthew Boote.\n",XSCRIPT_VERSION_MAJOR,XSCRIPT_VERSION_MINOR,XSCRIPT_VERSION_REVISION);
printf("This program comes with ABSOLUTELY NO WARRANTY; for details type `help warranty'.\n");
printf("This is free software, and you are welcome to redistribute it under certain conditions;\n");
printf("type `help copying' for details.\n");
printf("Type help for general help.\n");

while(1) {	
	SwitchToInteractiveModeBuffer();		/* switch to interactive mode buffer */

	if(block_statement_nest_count == 0) {
		printf(">");
	}
	else
	{
		printf("...");
	}

	fgets(linebuf,MAX_SIZE,stdin);			/* read line */

	RemoveNewline(linebuf);		/* remove newline characters */

	GetTokenCharacters(tokenchars);			/* get token characters */
	tc=TokenizeLine(linebuf,tokens,tokenchars);	/* tokenize line */

	/* If it's a function declaration, it must be handled differently:
	   The function declaration, the statements and the endfunction statement are stored in a different buffer
	   from the interactive mode buffer. */

	if(strcmpi(tokens[0],"FUNCTION") == 0) {
		block_statement_nest_count++;
		IsDeclaringFunction=TRUE;
	}
	else if(strcmpi(tokens[0],"ENDFUNCTION") == 0) {		/* end of function */
		sprintf(FunctionBufferPtr,"%s\n",linebuf);		/* add to buffer with newline */
		FunctionBufferPtr += (strlen(linebuf)+2);

		IsDeclaringFunction=FALSE;
		block_statement_nest_count--;

		tc=TokenizeLine(FunctionBufferAddress,tokens,tokenchars);	/* tokenize line */
		
		SetCurrentBufferPosition(FunctionBufferPtr);	/* Skip over function declaration */

		if(DeclareFunction(&tokens[1],tc-1) == -1) {		/* declare function */
			IsDeclaringFunction=FALSE;
			continue;
		}

		SwitchToInteractiveModeBuffer();		/* Point to start of buffer */
		continue;
	}
	
	if(IsDeclaringFunction == TRUE) {
		/* Copy input line to function buffer and resize the buffer */

		if(FunctionBufferAddress == NULL) {		/* first allocation */
			FunctionBufferAddress=malloc(strlen(linebuf));
			if(FunctionBufferAddress == NULL) {
				IsDeclaringFunction=FALSE;

				PrintError(NO_MEM);
				continue;
			}

			FunctionBufferPtr=FunctionBufferAddress;

			FunctionBufferSize=strlen(linebuf);
			OldFunctionBufferSize=FunctionBufferSize;

		}
		else					/* subsequent allocation */
		{
			OldFunctionBufferSize=FunctionBufferSize;

			FunctionBufferSize += strlen(linebuf);
			FunctionBufferAddress=realloc(FunctionBufferAddress,FunctionBufferSize);	/* resize buffer */

			if(FunctionBufferAddress == NULL) {
				IsDeclaringFunction=FALSE;

				PrintError(NO_MEM);
				continue;
			}

			FunctionBufferPtr=FunctionBufferAddress+OldFunctionBufferSize;
		}

		sprintf(FunctionBufferPtr,"%s\n",linebuf);		/* add to buffer with newline */
	}
	else
	{
		/* If it is not a function, then the statements are stored in a buffer and executed
		   after the outermost block statement is finished */

		sprintf(InteractiveModeBufferPosition,"%s\n",linebuf);		/* add to buffer with newline */

		InteractiveModeBufferPosition += strlen(InteractiveModeBufferPosition);	/* point to next statement */
		SetCurrentBufferPosition(InteractiveModeBufferPosition);

	
		if(IsBlockStatement(tokens[0]) == TRUE) {	/* if block statement */
			/* save start of block statement in linked list */

			if(blockstatementsave_head == NULL) {	/* first */
				blockstatementsave_head=malloc(sizeof(BLOCKSTATEMENTSAVE));

				if(blockstatementsave_head == NULL) {
					PrintError(NO_MEM);
					return(NO_MEM);
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

		if(block_statement_nest_count == 0) {	 /* if at end of entering statements */
			SwitchToInteractiveModeBuffer();		/* switch to interactive mode buffer */

			GetCurrentFunctionName(functionname);		/* get current function name */

			SetIsRunningFlag();

			do {
				if(GetIsRunningFlag() == FALSE)	break;		/* not running */
				SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));	/* read line from buffer */
	
				if(ExecuteLine(linebuf) == -1) {		/* run line */
					PrintError(GetLastError());
					break;
				}
			
				InteractiveModeBufferPosition += (strlen(linebuf)+1);
		 	} while(*InteractiveModeBufferPosition != 0);

			SetCurrentBufferPosition(SaveBufferAddress);	/* restore buffer address */
		   	memset(InteractiveModeBuffer,0,INTERACTIVE_BUFFER_SIZE);

		   	ClearIsRunningFlag();
			
		   	InteractiveModeBufferPosition=InteractiveModeBuffer;		/* set buffer positoon to start */
		}
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
cleanup();		/* deallocate lists */

exit(0);
}

/*
 * Continue statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns: error number on error, doesn't return on success
 *
 */
int continue_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(GetIsFileLoadedFlag() == FALSE) { 	/* no program running */
	PrintError(NO_RUNNING_PROGRAM);
	return(-1);
}

if(GetInteractiveModeFlag() == FALSE) {	/* not in interactive mode */
	PrintError(NOT_IN_INTERACTIVE_MODE);
	return(-1);
}

SetIsRunningFlag();
SwitchToFileBuffer();			/* switch to file buffer */

//asm("int $3");

siglongjmp(savestate,0);
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

if(GetInteractiveModeFlag() == FALSE) {	/* not in interactive mode */
	PrintError(NOT_IN_INTERACTIVE_MODE);
	return(-1);
}

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

if(GetInteractiveModeFlag() == FALSE) {	/* not in interactive mode */
	PrintError(NOT_IN_INTERACTIVE_MODE);
	return(-1);
}

GetCurrentFile(currentfile);		/* get current file */

if(strlen(currentfile) == 0) {		/* check if there is a file */
	printf("No file loaded\n");
	return(0);
}

return(ExecuteFile(currentfile,NULL));	/* run file */
}

/*
 * Trace statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns: nothing
 *
 */

void trace_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
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

return;
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
void sbreak_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {

if(tc < 2) {						/* Too few parameters */
	PrintError(SYNTAX_ERROR);
	return(-1);
}

set_breakpoint(atoi(tokens[2]),tokens[3]);

SetLastError(NO_ERROR);
return(0);
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
int cbreak_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(tc < 2) {						/* Not enough parameters */
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

clear_breakpoint(atoi(tokens[2]),tokens[3]);

SetLastError(NO_ERROR);
return(0);
}

/*
 * Switch to interactive mode buffer
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */
void SwitchToInteractiveModeBuffer(void) {
SetCurrentBufferPosition(InteractiveModeBuffer);
}

/*
 * backtrace command
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int backtrace_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
PrintBackTrace();
return(0);
}

/*
 * Free interactive mode buffer
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */
void FreeInteractiveModeBuffer(void) {
if(InteractiveModeBuffer != NULL) free(InteractiveModeBuffer);
return;
}

void PrintBackTrace(void) {
FUNCTIONCALLSTACK *stack;
int callstackcount=0;
vars_t *varnext;

stack=GetFunctionCallStackTop();

while(stack != NULL) {
	printf("#%d	%s(",callstackcount++,stack->name);

	/* display initial parameters with values */

	varnext=stack->initialparameters;	/* point to parameters */

	while(varnext != NULL) {
		if(varnext->val != NULL) {
			if(varnext->type_int == VAR_NUMBER) {				/* double precision */
				printf("%s=%.6g",varnext->varname,varnext->val->d);
			}
			else if(varnext->type_int == VAR_STRING) {			/* string */	
				printf("%s=\"%s\"",varnext->varname,varnext->val->s);
			}
			else if(varnext->type_int == VAR_INTEGER) {			/* integer */
				printf("%s=%d",varnext->varname,varnext->val->i);
       			}
			else if(varnext->type_int == VAR_SINGLE) {				/* single */	     
				printf("%s=%f",varnext->varname,varnext->val->f);
			}

			if((varnext != NULL) && (varnext->next != NULL)) printf(", ");
		}

		varnext=varnext->next;

  	}
	
	printf(") : %s (%s:%d)\n",stack->returntype,stack->moduleptr->modulename,stack->currentlinenumber);

	stack=stack->next;
}

}

