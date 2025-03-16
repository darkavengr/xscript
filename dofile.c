/*  XScript Version 0.0.1
   (C) Matthew Boote 2020

   This file is part of XScript.

   XScript is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the LNumberOfIncludedFilesense, or
   (at your option) any later version.

   XScript is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with XScript.  If not, see <https://www.gnu.org/lNumberOfIncludedFilesenses/>.
*/

/* File and statement processing functions  */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "errors.h"
#include "size.h"
#include "evaluate.h"
#include "modeflags.h"
#include "variablesandfunctions.h"
#include "dofile.h"
#include "debugmacro.h"

extern jmp_buf savestate;

int saveexprtrue=0;
functionreturnvalue retval;
char *CurrentBufferPosition=NULL;	/* current pointer in buffer - points to either FileBufferPosition or interactive mode buffer */
char *FileBufferPosition=NULL;
char *endptr=NULL;		/* end of buffer */
char *FileBuffer=NULL;		/* buffer */
int FileBufferSize=0;		/* size of buffer */
int NumberOfIncludedFiles=0;	/* number of included files */
char *TokenCharacters="+-*/<>=!%~|& \t()[],{};.#";
int Flags=0;
char *CurrentFile[MAX_SIZE];

/*
 * Load file
 *
 * In: char *filename		Filename of file to load
 *
 * Returns -1 on error or 0 on success
 *
 */
int LoadFile(char *filename) {
FILE *handle; 
int filesize;

sigsetjmp(savestate,1);		/* save current context */

handle=fopen(filename,"r");				/* open file */
if(!handle) {
	SetLastError(FILE_NOT_FOUND);
	SetLastError(-1);
}

sigsetjmp(savestate,1);		/* save current context */

fseek(handle,0,SEEK_END);				/* get file size */
filesize=ftell(handle);
fseek(handle,0,SEEK_SET);				/* seek back to start */

sigsetjmp(savestate,1);		/* save current context */
	
if(FileBuffer == NULL) {				/* first time */
	FileBuffer=malloc(filesize+1);			/* allocate buffer */

	if(FileBuffer == NULL) {
		SetLastError(NO_MEM);		/* no memory */
		return(-1);
	}
}
else
{
	if(realloc(FileBuffer,FileBufferSize+filesize) == NULL) SetLastError(NO_MEM);		/* resize buffer */
}

sigsetjmp(savestate,1);		/* save current context */

FileBufferPosition=FileBuffer;

if(fread(FileBuffer,filesize,1,handle) != 1) SetLastError(READ_ERROR);		/* read to buffer */
		
endptr=(FileBuffer+filesize);		/* point to end */
*endptr=0;			/* put null at end */

FileBufferSize += filesize;

strcpy(CurrentFile,filename);

sigsetjmp(savestate,1);		/* save current context */

SetIsFileLoadedFlag();
ClearIsRunningFlag();
SetLastError(0);
}

/*
 * Load and execute file
 *
 * In: char *filename		Filename of file to load or NULL to resume current file
 *
 * Returns -1 on error or 0 on success
 *
 */
int ExecuteFile(char *filename) {
char *linebuf[MAX_SIZE];
char *saveCurrentBufferPosition;
int returnvalue;
varval progname;

progname.s=malloc(strlen(filename));
if(progname.s == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

if(filename != NULL) {
	if(LoadFile(filename) == -1) {
		SetLastError(FILE_NOT_FOUND);
		return(-1);
	}

	strcpy(progname.s,filename);				/* update program name */
	UpdateVariable("PROGRAMNAME",NULL,&progname,0,0);

	SetCurrentFileBufferPosition(FileBuffer);
	SetCurrentBufferPosition(FileBuffer);
	SetIsFileLoadedFlag();
}

SetIsRunningFlag();
SwitchToFileBuffer();			/* switch to file buffer */

SetCurrentFunctionLine(1);

saveCurrentBufferPosition=GetCurrentBufferPosition();		/* save current pointer */
SetFunctionCallPtr(saveCurrentBufferPosition);		/* set start of current function to buffer start */

do {
	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,linebuf,LINE_SIZE);			/* get data */
	SetCurrentFileBufferPosition(CurrentBufferPosition);

	if(ExecuteLine(linebuf) == -1) {			/* run statement */

		strcpy(progname.s,"");				/* update program name */
		UpdateVariable("PROGRAMNAME",NULL,&progname,0,0);

		ClearIsRunningFlag();


		free(progname.s);
		return(-1);
	}

	if(GetIsRunningFlag() == FALSE) {
		strcpy(progname.s,"");				/* update program name */
		UpdateVariable("PROGRAMNAME",NULL,&progname,0,0);

		CurrentBufferPosition=saveCurrentBufferPosition;


		free(progname.s);

		SetLastError(NO_ERROR);	/* program ended */
		return(0);
	}

	memset(linebuf,0,MAX_SIZE);

	SetCurrentFunctionLine(GetCurrentFunctionLine()+1);

}    while(*CurrentBufferPosition != 0); 			/* until end */

CurrentBufferPosition=saveCurrentBufferPosition;

strcpy(progname.s,"");				/* update program name */
UpdateVariable("PROGRAMNAME",NULL,&progname,0,0);

free(progname.s);
SetLastError(NO_ERROR);
return(0);
}	

/*
 * Execute line
 *
 * In: char *lbuf		Line to process
 *
 * Returns -1 on error or 0 on success
 *
 */

int ExecuteLine(char *lbuf) {
char *tokens[MAX_SIZE][MAX_SIZE];
char *outtokens[MAX_SIZE][MAX_SIZE];
double exprone;
int tc;
varsplit split;
varsplit assignsplit;
varval val;
char c;
int vartype;
int count;
char *b;
char *d;
vars_t *varptr;
vars_t *assignvarptr;
int returnvalue;
char *functionname[MAX_SIZE];
int lc=GetCurrentFunctionLine();
int end;
int IsValid=FALSE;

GetCurrentFunctionName(functionname);

c=*lbuf;
if((c == '\r') || (c == '\n') || (c == 0)) {
	SetLastError(0);			/* blank line */
	return(0);
}

if(GetTraceFlag()) {		/* print statement if trace is enabled */
	printf("***** Tracing line %d in function %s: %s\n",GetCurrentFunctionLine(),functionname,lbuf);
}

if(strlen(lbuf) > 1) {
	b=lbuf+strlen(lbuf)-1;
	if((*b == '\n') || (*b == '\r')) *b=0;
}

while(*lbuf == ' ' || *lbuf == '\t') lbuf++;	/* skip white space */

memset(tokens,0,MAX_SIZE*MAX_SIZE);

tc=TokenizeLine(lbuf,tokens,TokenCharacters);			/* tokenize line */

/* remove comments */

for(count=0;count < tc;count++) {
	if(strcmp(tokens[count],"#") == 0) {	/* found comment */
		if(count == 0) {
			SetLastError(0);		/* ignore lines with only comments */
			return(0);
		}

		/* remove comments from array */

		for(end=count;end<tc;end++) {
			*tokens[end]=0;
		}

		tc=count;		/* set new end */
		break;
	}
}

//if(CheckSyntax(tokens,TokenCharacters,1,tc) == FALSE) {
//	SetLastError(SYNTAX_ERROR);		/* check syntax */
//	return(-1);
//}

if(IsStatement(tokens[0])) {
	returnvalue=CallIfStatement(tc,tokens); /* run if statement */

	if(returnvalue == -1) {
		PrintError(GetLastError());
		return(-1);
	}

	IsValid=TRUE;
}

/*
 *
 * assignment
 *
 */

for(count=1;count<tc;count++) {

	if((strcmp(tokens[count],"=") == 0) && (IsValid == FALSE)) {
		for(int countx=0;countx<tc;countx++) {
			printf("var=%s\n",tokens[countx]);
		}

		IsValid=TRUE;

 		ParseVariableName(tokens,0,count,&split);			/* split variable */  	

		returnvalue=SubstituteVariables(count+1,tc,tokens,outtokens);
 		if(returnvalue == -1) return(-1);

	 	if(strlen(split.fieldname) == 0) {			/* use variable name */
	 		vartype=GetVariableType(split.name);
		}
		else
		{
	 		vartype=GetFieldTypeFromUserDefinedType(split.name,split.fieldname);
			if(vartype == -1) {
				SetLastError(TYPE_FIELD_DOES_NOT_EXIST);
				return(-1);
			}
		}

		c=*outtokens[count+1];

		if((c != '"') && (vartype == VAR_STRING)) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}

		if( (c == '"') && ((vartype == VAR_STRING) || (vartype == -1))) {			/* string */  
			if(vartype == -1) CreateVariable(split.name,"STRING",split.x,split.y);		/* new variable */ 
		  		 
		  	ConatecateStrings(count+1,tc,outtokens,&val);					/* join all the strings on the line */

		  	UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);		/* set variable */
		  	SetLastError(0);
			return(-1);
		}

		/* number otherwise */

		 if( ((c == '"') && (vartype != VAR_STRING))) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}
	
		 if(IsValidExpression(tokens,0,returnvalue) == FALSE) {
			printf("assign expression invalid\n");

			SetLastError(INVALID_EXPRESSION);	/* invalid expression */
			return(-1);
		 }

		 exprone=EvaluateExpression(outtokens,0,returnvalue);

		// printf("exprone=%s %.6g\n",split.name,exprone);

		 if(vartype == VAR_NUMBER) {
	 		val.d=exprone;
	 	 }
		 else if(vartype == VAR_STRING) {
			returnvalue=SubstituteVariables(count+1,count+1,tokens,outtokens);
 			if(returnvalue == -1) return(-1);

		 	strcpy(val.s,outtokens[0]);  
		}
		else if(vartype == VAR_INTEGER) {
	 		val.i=exprone;
	 	}
	 	else if(vartype == VAR_SINGLE) {
	 		val.f=exprone;
	 	}
		else if(vartype == VAR_LONG) {
	 		val.l=exprone;
	 	}
	 	else if(vartype == VAR_UDT) {			/* user-defined type */	 
			ParseVariableName(tokens,count+1,tc,&assignsplit);		/* split variable */  	
 			varptr=GetVariablePointer(split.name);		/* point to variable entry */
			assignvarptr=GetVariablePointer(assignsplit.name);

			if((varptr == NULL) || (assignvarptr == NULL)) {
				SetLastError(VARIABLE_DOES_NOT_EXIST);
				return(-1);
			}
			   
			CopyUDT(assignvarptr->udt,assignvarptr->udt);		/* copy UDT */

			SetLastError(0);
			return(0);
 		}

		if(vartype == -1) {		/* new variable */ 	  
		 	val.d=exprone;

		 	CreateVariable(split.name,"DOUBLE",split.x,split.y);			/* create variable */
		 	UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);
	
		 	SetLastError(0);
			return(0);
		 }

		 UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);

		 SetLastError(0);
		 return(0);		 
	 }

}

/* call user function */

if((CheckFunctionExists(tokens[0]) != -1) && (IsValid == FALSE)) {	/* user function */
	CallFunction(tokens,0,tc);
	IsValid=TRUE;
} 

if(IsValid == FALSE) {
	SetLastError(INVALID_STATEMENT);
	return(-1);
}

if(check_breakpoint(GetCurrentFunctionLine(),functionname) == TRUE) {	/* if there is a breakpoint */
	printf("***** Reached breakpoint: function %s line %d\n",functionname,GetCurrentFunctionLine());

	ClearIsRunningFlag();
}

SetLastError(0);
return(0);
}

/*
 * Declare function statement
 *
 * In: tc Token count
 *     tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(DeclareFunction(&tokens[1],tc-1) == -1) return(-1);

SetLastError(0);
return(0);
} 

/*
 * Print statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
varval val;
int count;
int countx;
int returnvalue;
char *printtokens[MAX_SIZE][MAX_SIZE];
int endtoken;
bool IsInBracket;
vars_t *tokenvar;

sigsetjmp(savestate,1);		/* save current context */

for(count=1;count<tc;count++) {
	IsInBracket=FALSE;

	/* if string literal, string variable or function returning string */

	sigsetjmp(savestate,1);		/* save current context */

	for(endtoken=count;endtoken<tc;endtoken++) {
		/* is function parameter or array subscript */

		if((strcmp(tokens[endtoken],"(") == 0) || (strcmp(tokens[endtoken],"[") == 0)) IsInBracket=TRUE;														

		if((IsInBracket == FALSE) && (strcmp(tokens[endtoken],",") == 0)) break;		/* found separator not subscript */
	}

	sigsetjmp(savestate,1);		/* save current context */

	/* if printing array */

	if((GetVariableXSize(tokens[count]) > 0) || (GetVariableYSize(tokens[count]) > 0)) {		/* is array */
		if(IsInBracket == FALSE) {
			PrintError(MISSING_SUBSCRIPT);

			SetLastError(MISSING_SUBSCRIPT);
			return(-1);
		}
	}

	/* printing string */
	if(((char) *tokens[count] == '"') || (GetVariableType(tokens[count]) == VAR_STRING) || (CheckFunctionExists(tokens[count]) == VAR_STRING) ) {
		memset(printtokens,0,MAX_SIZE*MAX_SIZE);
	
		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);	
		if(returnvalue == -1) return(-1);		/* error occurred */

		count += ConatecateStrings(0,returnvalue,printtokens,&val);		/* join all the strings in the token */

		printf("%s ",val.s);
	}
	else
	{
		sigsetjmp(savestate,1);		/* save current context */

		memset(printtokens,0,MAX_SIZE*MAX_SIZE);

		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);
		if(returnvalue == -1) return(-1);

		//memcpy(printtokens,tokens,MAX_SIZE*4);

		if(IsValidExpression(tokens,count,endtoken-1) == FALSE) {
			SetLastError(INVALID_EXPRESSION);	/* invalid expression */
			return(-1);
		}

		retval.val.type=0;
		retval.val.d=EvaluateExpression(printtokens,0,returnvalue);
	
		/* if it's a condition print True or False */

		for(countx=count;countx<tc;countx++) {
			if((strcmp(tokens[countx],">") == 0) ||
			   (strcmp(tokens[countx],"<") == 0) ||
			   (strcmp(tokens[countx],"=") == 0) ||
			   (strcmp(tokens[countx],">=") == 0) ||
			   (strcmp(tokens[countx],"<=") == 0)) {

				retval.val.type=0;
	     			retval.val.d=EvaluateCondition(tokens,count,endtoken);

	     			retval.val.d == 1 ? printf("True ") : printf("False ");
	     			break;
	 		} 
		}

		if(countx == tc) printf("%.6g ",retval.val.d);	/* Not conditional */
	}

	count=endtoken;
}

sigsetjmp(savestate,1);		/* save current context */

printf("\n");

SetLastError(0);
return(0);
}

/*
 * Import statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *filename[MAX_SIZE];
char *extptr;

extptr=strstr(tokens[1],".");		/* get extension */

if(memcmp(extptr,".xsc",3) == 0) {	/* is source file */
	if(IsValidString(tokens[1]) == FALSE) {
		SetLastError(SYNTAX_ERROR);	/* is valid string */
		return(-1);
	}

	StripQuotesFromString(tokens[1],filename);		/* remove quotes from filename */

	if(IncludeFile(filename) == -1) return(-1);	/* including source file */

	SetLastError(0);
	return(0);
}

if(AddModule(tokens[1]) == -1) return(-1);		/* load module */

SetLastError(0);
return(0);
}

/*
 * If statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int count;
int countx;
char *d;
int exprtrue;
int returnvalue;

if(tc < 2) {
	SetLastError(SYNTAX_ERROR);					/* Too few parameters */
	return(-1);
}

SetFunctionFlags(IF_STATEMENT);

while(*CurrentBufferPosition != 0) {

	if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {  

		if(IsValidExpression(tokens,1,tc-1) == FALSE) {
			SetLastError(INVALID_EXPRESSION);	/* invalid expression */
			return(-1);
		 }

		exprtrue=EvaluateCondition(tokens,1,tc-1);
		if(exprtrue == -1) return(-1);

		if(exprtrue == 1) {
			saveexprtrue=exprtrue;

			do {
				sigsetjmp(savestate,1);		/* save current context */

		   		CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) {
					SetLastError(0);
					return(0);
				}

				if(ExecuteLine(buf) == -1) return(-1);

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) {
					SetLastError(SYNTAX_ERROR);
					return(-1);
				}
				
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					SetFunctionFlags(IF_STATEMENT);

					SetLastError(0);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
	 	}
	 	else
	 	{
			do {
	   			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) {
					SetLastError(0);
					return(0);
				}	

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) {
					SetLastError(SYNTAX_ERROR);
					return(-1);
				}
				
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					SetFunctionFlags(IF_STATEMENT);

					SetLastError(0);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
		}
	}


	if((strcmpi(tokens[0],"ELSE") == 0)) {

		if(saveexprtrue == 0) {
	    		do {
				sigsetjmp(savestate,1);		/* save current context */

	   			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) {
					SetLastError(0);
					return(0);
				}

				if(ExecuteLine(buf) == -1) return(-1);

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) {
					SetLastError(SYNTAX_ERROR);	
					return(-1);
				}
					
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					ClearFunctionFlags(IF_STATEMENT);

					SetLastError(0);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
		}
	}

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
}

SetLastError(ENDIF_NOIF);
return(-1);
}

/*
 * Endif statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number
 *
 */

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) == 0) {
	SetLastError(ENDIF_NOIF);
	return(-1);
}

SetLastError(0);
return(0);
}

/*
 * For statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */
int for_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int StartOfFirstExpression;
int StartOfSecondExpression;
int StartOfStepExpression;
int steppos;
double exprone;
double exprtwo;
varval loopcount;
int ifexpr;
char *d;
char *buf[MAX_SIZE];
int vartype;
varsplit split;
int returnvalue;
int lc=GetSaveInformationLineCount();
char *outtokens[MAX_SIZE][MAX_SIZE];
int endcount;

PushSaveInformation();

SetFunctionFlags(FOR_STATEMENT);

/* find start of first expression */

for(StartOfFirstExpression=1;StartOfFirstExpression<tc;StartOfFirstExpression++) {
	if(strcmpi(tokens[StartOfFirstExpression],"=") == 0) {
		StartOfFirstExpression++;
		break;
	}
}

if(StartOfFirstExpression == tc) {		/* no = */
	ClearFunctionFlags(FOR_STATEMENT);

	SetLastError(SYNTAX_ERROR);
	return(-1);
}

if(IsValidVariableOrKeyword(tokens[1]) == FALSE) {		/* check if variable name is valid */
	PopSaveInformation();

	ClearFunctionFlags(FOR_STATEMENT);

	SetLastError(SYNTAX_ERROR);
	return(-1);
}

//  0  1     2 3 4  5
// for count = 1 to 10

for(StartOfSecondExpression=3;StartOfSecondExpression<tc;StartOfSecondExpression++) {
	if(strcmpi(tokens[StartOfSecondExpression],"TO") == 0) {		/* found start of second expression */
		StartOfSecondExpression++;
		break;
	}
}

if(StartOfSecondExpression == tc) {
	PopSaveInformation();

	SetLastError(SYNTAX_ERROR);
	return(-1);
}


ParseVariableName(tokens,1,StartOfFirstExpression,&split);

for(StartOfStepExpression=1;StartOfStepExpression<tc;StartOfStepExpression++) {
	if(strcmpi(tokens[StartOfStepExpression],"step") == 0) {		/* found start of step expression */
		StartOfStepExpression++;
		break;
	}
}

if(StartOfStepExpression == tc) {		/* no step keyword */
	steppos=1;
}
else			/* have step keyword */
{	
	if(IsValidExpression(outtokens,StartOfStepExpression,tc) == FALSE) {
		PopSaveInformation();

		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
		return(-1);
	}

	returnvalue=SubstituteVariables(StartOfStepExpression,tc,tokens,outtokens);	/* substitute variables for step expression */
	if(returnvalue == -1) {
		PopSaveInformation();
		return(-1);
	}

	steppos=EvaluateExpression(tokens,0,returnvalue);		/* evaulate for step expression */
}

//  0   1    2 3 4  5
// for count = 1 to 10

if(IsValidExpression(tokens,StartOfFirstExpression,StartOfSecondExpression-1) == FALSE) {
	SetLastError(INVALID_EXPRESSION);
	return(-1);
}

if(IsValidExpression(tokens,StartOfSecondExpression,StartOfStepExpression) == FALSE) {
	SetLastError(INVALID_EXPRESSION);
	return(-1);
}

returnvalue=SubstituteVariables(StartOfFirstExpression,StartOfSecondExpression-1,tokens,outtokens);
if(returnvalue == -1) {
	PopSaveInformation();

	return(-1);
}

exprone=EvaluateExpression(outtokens,0,returnvalue);			/* start value */

returnvalue=SubstituteVariables(StartOfSecondExpression,StartOfStepExpression,tokens,outtokens);
if(returnvalue == -1) {
	PopSaveInformation();

	return(-1);
}

exprtwo=EvaluateExpression(outtokens,0,returnvalue);			/* end value */

if(IsVariable(split.name) == FALSE) {				/* create variable if it doesn't exist */
	CreateVariable(split.name,"DOUBLE",split.x,split.y);
}

vartype=GetVariableType(split.name);			/* check if string */

switch(vartype) {
	case -1:
		loopcount.d=exprone;
		vartype=VAR_NUMBER;
		break;

	case VAR_STRING: 
		SetLastError(TYPE_ERROR);
		return(-1);

	case VAR_NUMBER:
		loopcount.d=exprone;
		break;

	case VAR_INTEGER:
		loopcount.i=exprone;
		break;

	case VAR_SINGLE:
		loopcount.f=exprone;
		break;
}

UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y);			/* set loop variable to next */	

if(exprone >= exprtwo) {
	ifexpr=1;
}
else
{
	ifexpr=0;
}

SetFunctionFlags(FOR_STATEMENT);

SetSaveInformationBufferPointer(CurrentBufferPosition);

//printf("Start of for loop with variable %s\n",split.name);

while(1) {
	sigsetjmp(savestate,1);		/* save current context */

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */	

	if(ExecuteLine(buf) == -1) {
		PopSaveInformation();

		ClearIsRunningFlag();
		return(-1);
	}

	sigsetjmp(savestate,1);		/* save current context */

	if(GetIsRunningFlag() == FALSE) {
		SetLastError(NO_ERROR);	/* program halted */
		return(0);
	}

	sigsetjmp(savestate,1);		/* save current context */

	d=*buf+(strlen(buf)-1);
	if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
	if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

 	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

	sigsetjmp(savestate,1);

	if(strcmpi(tokens[0],"NEXT") == 0) {   
//		printf("Current loop end for variable %s\n",split.name);
	
		sigsetjmp(savestate,1);		/* save current context */
  
		SetCurrentFunctionLine(GetSaveInformationLineCount());
		lc=GetSaveInformationLineCount();

		sigsetjmp(savestate,1);		/* save current context */
  
	      /* increment or decrement counter */
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 1)) loopcount.d -= steppos;
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 0)) loopcount.d += steppos;      
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 1)) loopcount.i -= steppos;
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 0)) loopcount.i += steppos;      
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 1)) loopcount.f -=steppos;
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 0)) loopcount.f += steppos;      

//		printf("loopcount.d=%s %.6g %.6g\n",split.name,loopcount.d,exprtwo);

	      	UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y);			/* set loop variable to next */	

		sigsetjmp(savestate,1);		/* save current context */

	      	if(*CurrentBufferPosition == 0) {
	      		PopSaveInformation();				 
			ClearFunctionFlags(FOR_STATEMENT);

			CurrentBufferPosition=GetSaveInformationBufferPointer();

			SetLastError(SYNTAX_ERROR);
			return(-1);
	     	 }

		if(
       		( (vartype == VAR_NUMBER) && (ifexpr == 1) && (loopcount.d < exprtwo)) ||
		((vartype == VAR_NUMBER) && (ifexpr == 0) && (loopcount.d > exprtwo)) ||
		((vartype == VAR_INTEGER) && (ifexpr == 1) && (loopcount.i < exprtwo)) ||
	        ((vartype == VAR_INTEGER) && (ifexpr == 0) && (loopcount.i > exprtwo)) ||
	        ((vartype == VAR_SINGLE) && (ifexpr == 1) && (loopcount.f < exprtwo)) ||
	        ((vartype == VAR_SINGLE) && (ifexpr == 0) && (loopcount.f > exprtwo))
       		) break;

		CurrentBufferPosition=GetSaveInformationBufferPointer();			/* get pointer to start of for statement */
    	} 

	sigsetjmp(savestate,1);		/* save current context */
  
} 

sigsetjmp(savestate,1);		/* save current context */

//PopSaveInformation();
//CurrentBufferPosition=GetSaveInformationBufferPointer();

SetLastError(NO_ERROR);
return(0);
}

/*
 * Return statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;
int vartype;
int substreturnvalue=0;
char *outtokens[MAX_SIZE][MAX_SIZE];

SetFunctionFlags(FUNCTION_STATEMENT);

/* check return type */

for(count=1;count<tc;count++) {
	if((*tokens[count] >= '0' && *tokens[count] <= '9') && (GetFunctionReturnType() == VAR_STRING)) {
		SetLastError(TYPE_ERROR);
		return(-1);
	}

 	if((*tokens[count] == '"') && (GetFunctionReturnType() != VAR_STRING)) {
		SetLastError(TYPE_ERROR);
		return(-1);
	}

 	vartype=GetVariableType(tokens[count]);

 	if(vartype != -1) {
		if((GetFunctionReturnType() == VAR_STRING) && (vartype != VAR_STRING)) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}

  		if((GetFunctionReturnType() != VAR_STRING) && (vartype == VAR_STRING)) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}
	}
}

retval.val.type=GetFunctionReturnType();		/* get return type */

retval.has_returned_value=TRUE;				/* set has returned value flag */

if(GetFunctionReturnType() != VAR_STRING) {		/* returning number */

//	if(IsValidExpression(tokens,2,tc) == FALSE) {   /* invalid expression */
//		retval.has_returned_value=FALSE;	/* clear has returned value flag */

//		SetLastError(INVALID_EXPRESSION);
//		return(-1);	
//	}
}

if(GetFunctionReturnType() == VAR_STRING) {		/* returning string */
	ConatecateStrings(1,tc,tokens,&retval.val);		/* get strings */
}
else if(GetFunctionReturnType() == VAR_INTEGER) {		/* returning integer */
	substreturnvalue=SubstituteVariables(1,tc,tokens,outtokens);
	if(substreturnvalue == -1) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */
		return(-1);
	}
	
//	 if(IsValidExpression(tokens,1,tc) == FALSE) {
//		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
//		return(-1);
//	}
	
	retval.val.i=EvaluateExpression(outtokens,0,substreturnvalue);
}
else if(GetFunctionReturnType() == VAR_NUMBER) {		/* returning double */	 
	substreturnvalue=SubstituteVariables(1,tc,tokens,outtokens);
	if(substreturnvalue == -1) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */
		return(-1);
	}

	 if(IsValidExpression(outtokens,0,substreturnvalue) == FALSE) {
		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
		return(-1);
	}

	retval.val.d=EvaluateExpression(outtokens,0,substreturnvalue);
}
else if(GetFunctionReturnType() == VAR_SINGLE) {		/* returning single */
	substreturnvalue=SubstituteVariables(1,tc,tokens,outtokens);
	if(substreturnvalue == -1) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */

		return(-1);
	}

	 if(IsValidExpression(tokens,1,tc) == FALSE) {
		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
		return(-1);
	}
	
	retval.val.f=EvaluateExpression(outtokens,0,substreturnvalue);	
}

ReturnFromFunction();			/* return */

SetLastError(0);
return(0);
}

int get_return_value(varval *val) {
	val=&retval.val;
	SetLastError(0);
	return(0);
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if((GetFunctionFlags() & WHILE_STATEMENT) == 0) SetLastError(WEND_NOWHILE);

	SetLastError(0);
	return(0);
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if((GetFunctionFlags() & FOR_STATEMENT) == 0) SetLastError(NEXT_WITHOUT_FOR);

	SetLastError(0);
	return(0);
}

/*
 * While statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int exprtrue;
char *d;
int count;
char *condition_tokens[MAX_SIZE][MAX_SIZE];
int condition_tc;
int substreturnvalue;
char *saveBufferPosition;
int copycount;
int saveline=GetCurrentFunctionLine();

if(tc < 1) return(SYNTAX_ERROR);			/* Too few parameters */

count=0;

for(copycount=1;copycount<tc;copycount++) {
	strcpy(condition_tokens[count++],tokens[copycount]);
}

condition_tc=tc-1;

SetFunctionFlags(WHILE_STATEMENT);
saveBufferPosition=CurrentBufferPosition;		/* save current buffer position */
	
do {
     sigsetjmp(savestate,1);		/* save current context */

     CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
   
     if(IsValidExpression(condition_tokens,0,condition_tc) == FALSE) {
	SetLastError(INVALID_EXPRESSION);	/* invalid expression */
	return(-1);
     }

     exprtrue=EvaluateCondition(condition_tokens,0,condition_tc);			/* do condition */

     sigsetjmp(savestate,1);		/* save current context */

     tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
     if(tc == -1) {	
	SetLastError(SYNTAX_ERROR);
	return(-1);
     }
    
     substreturnvalue=ExecuteLine(buf);

     if(substreturnvalue != 0) {
     	ClearIsRunningFlag();
	return(substreturnvalue);
     }

     if(GetIsRunningFlag() == FALSE) return(NO_ERROR);	/* program ended */

     if((strcmpi(tokens[0],"WEND") == 0) && (exprtrue == 1)) {				/* at end of block, go back to start */
	     CurrentBufferPosition=saveBufferPosition;
     	     SetCurrentFunctionLine(saveline);		/* return line counter to start */
     }

} while(exprtrue == 1);
	
return(0);		      
}

/*
 * End statement
 *
 * In: tc Token count
 *     tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
/* If in interactive mode, return to command prompt, otherwise exit interpreter */

	if(GetInteractiveModeFlag() == TRUE) {
		ClearIsRunningFlag();

		SetLastError(atoi(tokens[1]));
		return(0);
	}
	else
	{
		exit(atoi(tokens[1]));
	}
}

/*
 * Else statement
 *
 * In: tc Token count
 *     tokens Tokens array
 *
 */

int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) {
	SetLastError(ELSE_WITHOUT_IF);
	return(-1);
}

SetLastError(0);
return(0);
}

/*
 * Elseif statement
 *
 * In: tc	Token count
		tokens	Tokens array
 *
 */

int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) SetLastError(ELSE_WITHOUT_IF);
}
/*
 * Endfunction statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & FUNCTION_STATEMENT) != FUNCTION_STATEMENT) {
	SetLastError(ENDFUNCTION_NO_FUNCTION);
	return(-1);
}

ClearFunctionFlags(FUNCTION_STATEMENT);
SetLastError(0);
}

/*
 * Exit statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
char *savebuffer;

if(strcmpi(tokens[1],"FOR") == 0) {
	if((GetFunctionFlags() & FOR_STATEMENT) == 0) {
		SetLastError(EXIT_FOR_WITHOUT_FOR);
		return(-1);
	}
}
else if(strcmpi(tokens[1],"WHILE") == 0) {
	if((GetFunctionFlags() & WHILE_STATEMENT) == 0)	{
		SetLastError(EXIT_WHILE_WITHOUT_WHILE);
		return(-1);
	}
}
else
{
	SetLastError(SYNTAX_ERROR);
	return(-1);
}

/* find end of loop */
while(*CurrentBufferPosition != 0) {
	savebuffer=CurrentBufferPosition;

	sigsetjmp(savestate,1);		/* save current context */

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,MAX_SIZE);			/* get data */

	TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

	ClearIsRunningFlag();

	if((strcmpi(tokens[1],"WEND") == 0) && (GetFunctionFlags() & WHILE_STATEMENT)){
	 	ClearFunctionFlags(WHILE_STATEMENT);
		SetLastError(0);
		return(0);
	}

	if((strcmpi(tokens[1],"FOR") == 0) && (GetFunctionFlags() & FOR_STATEMENT)){
	 	ClearFunctionFlags(FOR_STATEMENT);
		SetLastError(0);
		return(0);
	}

   } 

SetLastError(0);
return(0);
}

/*
 * Declare statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
varsplit split;
int vartype;
int count;

ParseVariableName(tokens,1,tc,&split);

/* if there is a type in the declare statement */

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"AS") == 0) break;
}

if(count < tc) {		/* found type */
	vartype=CheckVariableType(tokens[count+1]);	/* get variable type */
	if(vartype == -1) {
		SetLastError(INVALID_VARIABLE_TYPE);
		return(-1);
	}

	retval.val.type=vartype;
	retval.val.i=CreateVariable(split.name,tokens[count+1],split.x,split.y);
}
else
{
	retval.val.type=VAR_NUMBER;
	retval.val.i=CreateVariable(split.name,"DOUBLE",split.x,split.y);
}

if(retval.val.i == -1) return(-1);

SetLastError(0);
return(0);
}

/*
 * Iterate statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error on error or 0 on success
 *
 */

int iterate_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {


if(((GetFunctionFlags() & FOR_STATEMENT)) || ((GetFunctionFlags() & WHILE_STATEMENT))) {
	CurrentBufferPosition=GetSaveInformationBufferPointer();

	SetLastError(0);
	return(0);
}

SetLastError(CONTINUE_NO_LOOP);
return(-1);
}

/*
 * Type statement
 *
 * In: tc Token count
 *	tokens Tokens array
 *
 * Returns error on error or 0 on success
 *
 */
int type_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *typetokens[MAX_SIZE][MAX_SIZE];
int typetc;
int count;
char *buf[MAX_SIZE];
UserDefinedTypeField *fieldptr;
UserDefinedType addudt;
varsplit split;

if(tc < 1) {
	SetLastError(SYNTAX_ERROR);			/* Too few parameters */
	return(-1);
}

if((GetUDT(tokens[1]) != NULL) || (IsValidVariableType(tokens[1]) != -1)) {
	SetLastError(TYPE_EXISTS);		/* If type exists */
	return(-1);
}

/* create user-defined type entry */

strcpy(addudt.name,tokens[1]);

addudt.field=malloc(sizeof(UserDefinedTypeField));	/* add first field */
if(addudt.field == NULL) {
	SetLastError(NO_MEM); 
	return(-1);
}
	
fieldptr=addudt.field;

/* add user-defined type fields */
	
do {

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	typetc=TokenizeLine(buf,typetokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) {
		SetLastError(SYNTAX_ERROR); 
		return(-1);
	}

	if(strcmpi(typetokens[0],"ENDTYPE") == 0) {		/* end of statement */
		AddUserDefinedType(&addudt);			/* add user-defined type */

		SetLastError(0);
		return(0);
	}
	if(strcmpi(typetokens[1],"AS") != 0) {

		SetLastError(SYNTAX_ERROR); /* missing as */
		return(-1);
	}

/* add field to user defined type */
	
	ParseVariableName(typetokens,0,typetc,&split);		/* split variable name */

	strcpy(fieldptr->fieldname,split.name);		/* copy name */
	
	fieldptr->xsize=split.x;				/* get x size of field variable */
	fieldptr->ysize=split.y;				/* get y size of field variable */
	fieldptr->type=IsValidVariableType(typetokens[2]);

/* get type of field variable */

	if(fieldptr->type == -1) {
		SetLastError(TYPE_ERROR); 	/* is valid type */
		return(-1);
	}

	if(*CurrentBufferPosition == 0) break;		/* at end */
	
/* add link to next field */
	fieldptr->next=malloc(sizeof(UserDefinedTypeField));
	if(fieldptr->next == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
	
	fieldptr=fieldptr->next;

} while(*CurrentBufferPosition != 0);

SetLastError(TYPE_NO_END_TYPE);
return(-1);
}

/*
 * try...catch statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int try_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int returnvalue;
char *trytokens[MAX_SIZE][MAX_SIZE];
	
while(*CurrentBufferPosition != 0) {

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */

	if(strcmpi(trytokens[0],"CATCH") == 0) {	/* reached catch statement without error occurring in try block */
		while(*CurrentBufferPosition != 0) {
			sigsetjmp(savestate,1);		/* save current context */

			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

			tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */
	
			if(strcmpi(trytokens[0],"ENDTRY") == 0) {
				SetLastError(0);
				return(0);
			}
		}

		SetLastError(TRY_WITHOUT_ENDTRY);
		return(-1);
	}


	if(ExecuteLine(buf) == -1) {			/* run statement */
		/* error occurred */

		while(*CurrentBufferPosition != 0) {	/* find catch block */
			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

			tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */

			if(strcmpi(trytokens[0],"CATCH") == 0) {	/* found catch block */

			/* run catch statements */

				while(*CurrentBufferPosition != 0) {
					CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
					tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */

					if(strcmpi(trytokens[0],"ENDTRY") == 0) {
						SetLastError(0);		/* at end of catch block */
						return(0);
					}

					if(ExecuteLine(buf) == -1) return(-1);	/* run statement and return if error */
				}
			}
		}

		SetLastError(TRY_WITHOUT_CATCH);		/* no catch block */
		return(-1);
	}
}

SetLastError(TRY_WITHOUT_ENDTRY);		/* no endtry statement */
return(-1);
}

/*
 * endtry statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int endtry_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
SetLastError(ENDTRY_WITHOUT_TRY);
return(-1);
}

/*
 * catch statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int catch_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
SetLastError(CATCH_WITHOUT_TRY);
return(-1);
}

/*
 * Resize statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int resize_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
vars_t *var;
varsplit split;

if(tc < 3) {
	SetLastError(SYNTAX_ERROR);			/* Too few parameters */
	return(-1);
}

var=GetVariablePointer(tokens[1]);		/* get pointer to variable */
if(var == NULL) {
	SetLastError(VARIABLE_DOES_NOT_EXIST);
	return(-1);
}
if((var->xsize == 0) && (var->ysize == 0)) {
	SetLastError(NOT_ARRAY);
	return(-1);
}

ParseVariableName(tokens,1,tc,&split);		/* parse variable name */

SetLastError(ResizeArray(split.name,split.x,split.y));
return(-1);
}
/*
 * Help command
 *
 * In: tc Token count
 *	tokens Tokens array
 *
 * Returns error on error or 0 on success
 *
 */
int help_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *dirname[MAX_SIZE];

GetExecutableDirectoryName(dirname);

if(DisplayHelp(dirname,tokens[1]) == -1) return(-1);		/* display help */

SetLastError(0);
return(0);
}

/*
 * Non-statement keyword as statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns nothing
 *
 */

int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
SetLastError(SYNTAX_ERROR);
return(-1);
}

/*
 * Tokenize string
 *
 * In: char *linebuf			Line to tokenize
		char *tokens[MAX_SIZE][MAX_SIZE]	Token array output
 *
 * Returns -1 on error or token count on success
 *
 */

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split) {
char *token;
int tc;
int count;
char *d;
char *s;
char *nexttoken;
int IsSeperator;
char *b;

token=linebuf;

while(*token == ' ' || *token == '\t') token++;	/* skip leading whitespace characters */

/* tokenize line */

	tc=0;
	
	d=tokens[0];
	memset(d,0,MAX_SIZE);				/* clear line */
	
	while(*token != 0) {
		IsSeperator=FALSE;

		if(*token == '"' ) {		/* quoted text */ 
	   		*d++=*token++;

	   		while(*token != 0) {
	    			*d++=*token++;

	    			if(*(d-1) == '"') break;		/* quoted text */	

	  		}

	  		tc++;
	 	}
	 	else
	 	{
	  		s=split;

	  		while(*s != 0) {
	    			if(*token == *s) {		/* token found */
	   
		    		b=token;
		   	 	b--;

		    		if(strlen(tokens[tc]) != 0) tc++;
	    
		    		IsSeperator=TRUE;
		    		d=tokens[tc]; 			

		    		memset(d,0,MAX_SIZE);				/* clear line */

		    		if(*token != ' ') {
		      			*d=*token++;
	     		     		d=tokens[++tc]; 				      
		    		}
		    		else
		    		{
		      			token++;
		    		}
	   	}

	 	s++;
	 }

	 if(IsSeperator == FALSE) *d++=*token++; /* non-token character */
	}
}

if(strlen(tokens[tc]) > 0) tc++;		/* if there is data in the last token, increment the counter so it is accounted for */

return(tc);
}

/*
 * Check if is seperator
 *
 * In: token		Token to check
		sep		Seperator characters to check against
 *
 * Returns TRUE or FALSE
 *
 */
int IsSeperator(char *token,char *sep) {
char *s;

if(*token == 0) return(TRUE);
	
s=sep;

while(*s != 0) {
	if(*s++ == *token) return(TRUE);
}

if(IsStatement(token) == TRUE) return(TRUE);

return(FALSE);
}

/*
 * Check syntax
 *
 * In: tokens	Tokens to check
		separators	Separator characters to check against
		start		Array start
		end		Array end
 *
 * Returns TRUE or FALSE
 *
 */ 
int CheckSyntax(char *tokens[MAX_SIZE][MAX_SIZE],char *separators,int start,int end) {
int count;
int bracketcount=0;
int squarebracketcount=0;
bool IsInBracket=FALSE;
int statementcount=0;
int commacount=0;
int variablenameindex=0;
int starttoken;

/* check if brackets are balanced */

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"(") == 0) {
		commacount=0;

		if(IsInBracket == FALSE) variablenameindex=(count-1);			/* save name index */

		IsInBracket=TRUE;
		bracketcount++;
	}

	if(strcmp(tokens[count],")") == 0) {
		IsInBracket=FALSE;
		bracketcount--;
	}

	if(strcmp(tokens[count],"[") == 0) {
		IsInBracket=TRUE;
		squarebracketcount++;
	}

	if(strcmp(tokens[count],"]") == 0) {
		IsInBracket=FALSE;
		squarebracketcount--;
	}

	/* check if number of commas is correct for array or function call */
	if(strcmp(tokens[count],",") == 0) {		/* list of expressions */
		commacount++;

		if(IsInBracket == FALSE) return(FALSE);	/* list not in brackets */

		if((bracketcount > 1) || (squarebracketcount > 1)) return(FALSE);

		if(IsVariable(tokens[variablenameindex]) == TRUE) {
			if(commacount >= 2) return(FALSE); /* too many commas for array */
		}
	}
}

if((bracketcount != 0) || (squarebracketcount != 0)) return(FALSE);

for(count=start;count<end;count++) {

/* check if two separators are together */

	if(*tokens[count] == 0) break;

	if((strcmpi(tokens[0],"HELP") != 0) && (strcmpi(tokens[0],"PRINT") != 0)  && (strcmpi(tokens[0],"EXIT") != 0)) {
		if((IsSeperator(tokens[count],separators) == 1) && (*tokens[count] == 0)) return(TRUE);
	   
		if((IsSeperator(tokens[count],separators) == 1) && (IsSeperator(tokens[count+1],separators) == 1)) {
		/* brackets can be next to separators */

			if((strcmp(tokens[count],"(") == 0 && strcmp(tokens[count+1],"(") != 0)) return(TRUE);
			if( (strcmp(tokens[count],"(") == 0 && strcmp(tokens[count+1],"(") != 0)) return(TRUE);
			if( (strcmp(tokens[count],")") == 0 && strcmp(tokens[count+1],")") != 0)) return(TRUE);
			if( (strcmp(tokens[count],"(") != 0 && strcmp(tokens[count+1],"(") == 0)) return(TRUE);
			if( (strcmp(tokens[count],")") != 0 && strcmp(tokens[count+1],")") == 0)) return(TRUE);
			if( (strcmp(tokens[count],"[") == 0 && strcmp(tokens[count+1],"(") == 0)) return(TRUE);
			if( (strcmp(tokens[count],")") == 0 && strcmp(tokens[count+1],"]") == 0)) return(TRUE);
			if( (strcmp(tokens[count],")") == 0) && (*tokens[count+1] == 0)) return(TRUE);
			if( (strcmp(tokens[count],"]") == 0) && (*tokens[count+1] == 0)) return(TRUE);

			return(FALSE);
		}
	}
}

  /* check if two non-separator tokens are next to each other */

for(count=start;count<end;count++) {   
	if((IsSeperator(tokens[count],separators) == 0) && (count < end) && (IsSeperator(tokens[count+1],separators) == 0)) return(FALSE);
}

for(count=start+1;count<end;count++) {   		/* check if using keyword in statement */

	if((IsStatement(tokens[count]) == TRUE) && (strcmpi(tokens[0],"HELP") != 0)  && (strcmpi(tokens[0],"PRINT") != 0)) {
  		return(FALSE);
	}
}

return(TRUE);
}

	 
/*
 * Convert to uppercase
 *
 * In: char *token	String to convert
 *
 * Returns: nothing
 *
 */

void touppercase(char *token) {
	char *z;
	char c;

	z=token;

	while(*z != 0) { 	/* until end */
	 if(*z >= 'a' &&  *z <= 'z') {				/* convert to lower case if upper case */
	  *z -= 32;
	 }

	 z++;
	}
}

/*
 * Read line from buffer
 *
 * In: char *buf	Buffer to read from
		char *linebuf	Buffer to store line
		int size		Maximum size of line
 *
 * Returns -1 on error or address of next address in buffer for success
 *
 */

char *ReadLineFromBuffer(char *buf,char *linebuf,int size) {
int count=0;
char *lineptr=linebuf;
char *b;
char *z;
char *l;

memset(linebuf,0,size);

l=linebuf;

do {
	 if(count++ == size) break;

	 *l++=*buf++;
	 b=buf;
	 b--;

	 if(*b == 0) break;
	 if(*b == '\n' || *b == '\r') break;

} while(*b != 0);		/* until end of line */


b=linebuf;
b += strlen(linebuf);
b--;

*b=0;

return(buf);			/* return new position */
}

/*
 * Compare string case insensitively
 *
 * In: char *source		First string
		char *dest		Second string
 *
 * Returns: 0 if matches, positive or negative number otherwise
 *
 */
int strcmpi(char *source,char *dest) {
char a,b;
char *sourcetemp[MAX_SIZE];
char *desttemp[MAX_SIZE];

/* create copies of the string and convert them to uppercase */

memset(sourcetemp,0,MAX_SIZE);
memset(desttemp,0,MAX_SIZE);

strcpy(sourcetemp,source);
strcpy(desttemp,dest);

touppercase(sourcetemp);
touppercase(desttemp);

return(strcmp(sourcetemp,desttemp));		/* return result of string comparison */
}

char *GetCurrentBufferAddress(void) {
return(CurrentBufferPosition);
}

char *SetCurrentBufferAddress(char *addr) {
CurrentBufferPosition=addr;
}

void SetIsRunningFlag(void) {
Flags |= IS_RUNNING_FLAG;
}

void ClearIsRunningFlag(void) {
Flags &= ~IS_RUNNING_FLAG;
}

int GetIsRunningFlag(void) {
return((Flags & IS_RUNNING_FLAG) >> 1);
}

void SetIsFileLoadedFlag(void) {
Flags |= IS_FILE_LOADED_FLAG;
}

int GetIsFileLoadedFlag(void) {
return((Flags & IS_FILE_LOADED_FLAG) >> 2);
}

int GetInteractiveModeFlag(void) {
return((Flags & INTERACTIVE_MODE_FLAG));
}

void SetInteractiveModeFlag(void) {
Flags |= INTERACTIVE_MODE_FLAG;
}

void ClearInteractiveModeFlag(void) {
Flags &= ~INTERACTIVE_MODE_FLAG;
}

void GetCurrentFile(char *buf) {
strcpy(buf,CurrentFile);
}

void SetCurrentFile(char *buf) {
strcpy(CurrentFile,buf);
}

void SetCurrentBufferPosition(char *pos) {
CurrentBufferPosition=pos;
}

char *GetCurrentFileBufferPosition(void) {
return(FileBufferPosition);
}

void SetCurrentFileBufferPosition(char *pos) {
FileBufferPosition=pos;
}

char *GetCurrentBufferPosition(void) {
return(CurrentBufferPosition);
}

void SetTraceFlag(void) {
Flags |= TRACE_FLAG;
}	

void ClearTraceFlag(void) {
Flags &= ~TRACE_FLAG;
}	

int GetTraceFlag(void) {
return((Flags & TRACE_FLAG) >> 3);
}

void GetTokenCharacters(char *tbuf) {
strcpy(tbuf,TokenCharacters);
}

void SwitchToFileBuffer(void) {
CurrentBufferPosition=FileBufferPosition;
}

int IncludeFile(char *filename) {
struct stat includestat;
char *tempbuf;
FILE *handle;
size_t newptr;
char *oldtextptr;

if(stat(filename,&includestat) == -1) {
	SetLastError(FILE_NOT_FOUND);
	return(-1);
}

handle=fopen(filename,"rb");
if(!handle) {
	SetLastError(FILE_NOT_FOUND);
	return(-1);
}

/* replace include statement with file contents */

tempbuf=malloc(includestat.st_size);		/* allocate temporary buffer */
strcpy(tempbuf,CurrentBufferPosition);		/* save from current buffer to end */

/* find relative distance from start of buffer and apply it to new buffer pointer */

newptr=(CurrentBufferPosition-FileBuffer);	/* find distance from start of buffer */
if(realloc(FileBuffer,(FileBufferSize+includestat.st_size)) == NULL) {
	SetLastError(NO_MEM);	/* resize file buffer */
	return(-1);
}

CurrentBufferPosition=(FileBuffer+newptr);	/* new file buffer position */

/* find start of include statement so it can be overwritten */

CurrentBufferPosition -= 2;		/* skip newline */

while(CurrentBufferPosition != FileBuffer) {
	if(*CurrentBufferPosition-- == '\n') break;
}

if(fread(CurrentBufferPosition,includestat.st_size,1,handle) != 1) {
	SetLastError(READ_ERROR);		/* read include file to buffer */
	return(-1);
}


/* copy text after included file */

oldtextptr=(CurrentBufferPosition+includestat.st_size)-1;
strcpy(oldtextptr,tempbuf);

oldtextptr += includestat.st_size;		/* point to end */

//CurrentBufferPosition=(oldtextptr-2);
free(tempbuf);

fclose(handle);

SetLastError(0);
return(0);
}

int IsValidString(char *str) {
char *s;

if(*str != '"') return(FALSE);

s=(str+strlen(str))-1;		/* point to end */

if(*s != '"') return(FALSE);

return(TRUE);
}

void StripQuotesFromString(char *str,char *buf) {
char *s;
char *b;

if(IsValidString(str) == FALSE) return;		/* not valid string */

/* copy filename without quotes */

s=str;
s++;

b=buf;

while(*s != '"') *b++=*s++;	/* copy character */

return;
}

/*
 * Free file buffer
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */
void FreeFileBuffer(void) {
if(FileBuffer != NULL) free(FileBuffer);
}

