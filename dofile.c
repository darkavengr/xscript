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
#include "module.h"
#include "variablesandfunctions.h"
#include "dofile.h"
#include "debugmacro.h"

extern jmp_buf savestate;
extern char *vartypenames[];

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
int CurrentLine=0;
char *ConditionCharacters="<>=";

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
MODULES ModuleEntry;

sigsetjmp(savestate,1);		/* save current context */

handle=fopen(filename,"r");				/* open file */
if(!handle) {
	SetLastError(FILE_NOT_FOUND);
	return(-1);
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
	if(realloc(FileBuffer,FileBufferSize+filesize) == NULL) {
		SetLastError(NO_MEM);		/* resize buffer */
		return(-1);
	}
}

sigsetjmp(savestate,1);		/* save current context */

FileBufferPosition=FileBuffer;

if(fread(FileBuffer,filesize,1,handle) != 1) {
	SetLastError(IO_ERROR);		/* read to buffer */
	return(-1);
}
		
endptr=(FileBuffer+filesize);		/* point to end */
*endptr=0;			/* put null at end */

FileBufferSize += filesize;

strcpy(CurrentFile,filename);

sigsetjmp(savestate,1);		/* save current context */

SetIsFileLoadedFlag();
ClearIsRunningFlag();

/* add module entry */
strcpy(ModuleEntry.modulename,filename);	/* module filename */
ModuleEntry.flags=MODULE_SCRIPT;		/* module type */
ModuleEntry.StartInBuffer=FileBuffer;		/* start adress of module in buffer */

ModuleEntry.EndInBuffer=FileBuffer;		/* end adress of module in buffer */
ModuleEntry.EndInBuffer += filesize;
AddToModulesList(&ModuleEntry);

SetLastError(0);
return(0);
}

/*
 * Load and execute file
 *
 * In: char *filename		Filename of file to load or NULL to resume current file
 *
 * Returns -1 on error or 0 on success
 *
 */
int ExecuteFile(char *filename,char *args) {
char *linebuf[MAX_SIZE];
char *saveCurrentBufferPosition;
int returnvalue;
varval progname;
MODULES ModuleEntry;

progname.s=malloc(strlen(filename)+1);
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
	UpdateVariable("PROGRAMNAME",NULL,&progname,0,0,0,0);

	SetCurrentFileBufferPosition(FileBuffer);
	SetCurrentBufferPosition(FileBuffer);
	SetIsFileLoadedFlag();
}

InitializeMainFunction(filename,args);

SetIsRunningFlag();
SwitchToFileBuffer();			/* switch to file buffer */

SetCurrentFunctionLine(1);

saveCurrentBufferPosition=GetCurrentBufferPosition();		/* save current pointer */

SetFunctionCallPtr(saveCurrentBufferPosition);		/* set start of current function to buffer start */

SetCurrentFile(filename);			/* set current executing file */

do {
	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,linebuf,LINE_SIZE);			/* get data */
	SetCurrentFileBufferPosition(CurrentBufferPosition);

	if(ExecuteLine(linebuf) == -1) {			/* run statement */

		strcpy(progname.s,"");				/* update program name */
		UpdateVariable("PROGRAMNAME",NULL,&progname,0,0,0,0);

		ClearIsRunningFlag();

		free(progname.s);
		return(-1);
	}

	if(GetIsRunningFlag() == FALSE) {
		strcpy(progname.s,"");				/* update program name */
		UpdateVariable("PROGRAMNAME",NULL,&progname,0,0,0,0);

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
UpdateVariable("PROGRAMNAME",NULL,&progname,0,0,0,0);

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
int vartype;
int count;
vars_t *varptr;
vars_t *assignvarptr;
int returnvalue;
char *filename[MAX_SIZE];
int end;
int IsValid=FALSE;

GetCurrentFile(filename);	/* get name of current file */

if( (((char) *lbuf) == '\r') || (((char) *lbuf) == '\n') || (((char) *lbuf) == 0)) { 			/* blank line */
	SetLastError(0);
	return(0);
}

if(GetTraceFlag()) {		/* print statement if trace is enabled */
	printf("***** Tracing line %d in function %s: %s\n",GetCurrentFunctionLine(),filename,lbuf);
}

RemoveNewline(lbuf);		/* remove newline from line */

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

if(IsStatement(tokens[0])) {
	if(CallIfStatement(tc,tokens) == -1) return(-1); /* run if statement */

	IsValid=TRUE;
}

/*
 *
 * assignment
 *
 */

for(count=1;count<tc;count++) {

	if((strcmp(tokens[count],"=") == 0) && (IsValid == FALSE)) {
		IsValid=TRUE;

 		if(ParseVariableName(tokens,0,count,&split) == -1) return(-1);		/* split variable */  	

		if(IsValidVariableOrKeyword(split.name) == FALSE) {	/* Variable name is invalid */
			SetLastError(SYNTAX_ERROR);
			return(-1);
		}

		if((strlen(split.fieldname) > 0) && (IsValidVariableOrKeyword(split.fieldname) == FALSE)) {	/* if there is a field name, check its validity */
			SetLastError(SYNTAX_ERROR);
			return(-1);
		}

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

		if((((char) *outtokens[count+1]) != '"') && (vartype == VAR_STRING)) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}

		if( (((char) *tokens[count+1]) == '"') && ((vartype == VAR_STRING) || (vartype == -1))) {			/* string */  
			if(vartype == -1) CreateVariable(split.name,"STRING",split.x,split.y);		/* new variable */ 
		  		 
		  	ConatecateStrings(count+1,tc,tokens,&val);					/* join all the strings on the line */

			/* set variable */
		  	if(UpdateVariable(split.name,split.fieldname,&val,split.x,split.y,split.fieldx,split.fieldy) == -1) return(-1);

		  	SetLastError(0);
			return(0);
		}

		/* number otherwise */

		 if( ((((char) *outtokens[count+1]) == '"') && (vartype != VAR_STRING))) {
			SetLastError(TYPE_ERROR);
			return(-1);
		}
	
		 if(IsValidExpression(tokens,0,returnvalue) == FALSE) {
			SetLastError(INVALID_EXPRESSION);	/* invalid expression */
			return(-1);
		 }

		 exprone=EvaluateExpression(outtokens,0,returnvalue);

		 if(vartype == VAR_NUMBER) {
	 		val.d=exprone;
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
			if(ParseVariableName(tokens,count+1,tc,&assignsplit) == -1) return(-1);		/* split variable */

 			varptr=GetVariablePointer(split.name);		/* point to variable entry */
			assignvarptr=GetVariablePointer(assignsplit.name);

			if((varptr == NULL) || (assignvarptr == NULL)) {
				SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
				return(-1);
			}
			   
			CopyUDT(assignvarptr->udt,assignvarptr->udt);		/* copy UDT */

			SetLastError(0);
			return(0);
 		}
		else if(vartype == -1) {		/* new variable */ 	  
		 	val.d=exprone;

			if((split.x > 1) || (split.y) > 1) {		/* can't create array by assignment */
				SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
				return(-1);
			}
			 
		 	if(CreateVariable(split.name,"DOUBLE",1,1) == -1) return(-1);		/* create variable */
		 	if(UpdateVariable(split.name,split.fieldname,&val,0,0,0,0) == -1) return(-1);
	
		 	SetLastError(0);
			return(0);
		 }

		 UpdateVariable(split.name,split.fieldname,&val,split.x,split.y,split.fieldx,split.fieldy);

		 SetLastError(0);
		 return(0);		 
	 }

}

/* call user function */

if((CheckFunctionExists(tokens[0]) != -1) && (IsValid == FALSE)) {
	if(CallFunction(tokens,0,tc) == -1) return(-1);

	IsValid=TRUE;
} 

if(IsValid == FALSE) {
	SetLastError(INVALID_STATEMENT);
	return(-1);
}

if(check_breakpoint(GetCurrentFunctionLine(),filename) == TRUE) {	/* if there is a breakpoint */
	printf("***** Reached breakpoint: %s line %d\n",filename,GetCurrentFunctionLine());

	ClearIsRunningFlag();
}

SetLastError(0);
return(0);
}

/*
 * Declare function statement
 *
 * In: tc Token count
 *     tokens Token array
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
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
varval val;
int count;
int exprpos;
int returnvalue;
char *printtokens[MAX_SIZE][MAX_SIZE];
int endtoken;
bool IsInBracket;
vars_t *tokenvar;
char *tempstring[MAX_SIZE];
int IsCondition;

sigsetjmp(savestate,1);		/* save current context */

for(count=1;count<tc;count++) {
	IsInBracket=FALSE;
	IsCondition=FALSE;

	/* if string literal, string variable or function returning string */

	sigsetjmp(savestate,1);		/* save current context */

	for(endtoken=count;endtoken<tc;endtoken++) {
		/* is function parameter or array subscript */

		if((strcmp(tokens[endtoken],"(") == 0) || (strcmp(tokens[endtoken],"[") == 0)) IsInBracket=TRUE;														
		if((strcmp(tokens[endtoken],")") == 0) || (strcmp(tokens[endtoken],"]") == 0)) IsInBracket=FALSE;														

		if((IsInBracket == FALSE) && (strcmp(tokens[endtoken],",") == 0)) break;		/* found separator not subscript */
	}


	sigsetjmp(savestate,1);		/* save current context */

	/* printing string */
	if(((char) *tokens[count] == '"') || (GetVariableType(tokens[count]) == VAR_STRING) || (CheckFunctionExists(tokens[count]) == VAR_STRING) ) {
		memset(printtokens,0,MAX_SIZE*MAX_SIZE);
	
		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);	
		if(returnvalue == -1) return(-1);		/* error occurred */

		count += ConatecateStrings(0,returnvalue,printtokens,&val);		/* join all the strings in the token */

		StripQuotesFromString(val.s,tempstring);	/* remove quotes from string */
		printf("%s",tempstring);
	}
	else
	{
		sigsetjmp(savestate,1);		/* save current context */

		memset(printtokens,0,MAX_SIZE*MAX_SIZE);
	
		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);
		if(returnvalue == -1) return(-1);

		if(returnvalue > 0) {
			retval.val.type=0;
			retval.val.d=EvaluateExpression(printtokens,0,returnvalue);

			/* if it's a condition print True or False */
	
			for(exprpos=count;exprpos<tc;exprpos++) {
				if( (strcmp(tokens[exprpos],"=") == 0) ||
				   (strcmp(tokens[exprpos],"!=") == 0) || 
				   (strcmp(tokens[exprpos],">=") == 0) || 
				   (strcmp(tokens[exprpos],"<=") == 0) ||
				   (strcmp(tokens[exprpos],">") == 0) ||
				   (strcmp(tokens[exprpos],"<") == 0)) {

					retval.val.type=0;
		     			retval.val.d=EvaluateCondition(tokens,count,endtoken);
	
		     			retval.val.d == 1 ? printf("True ") : printf("False ");

					count=endtoken+1;
					IsCondition=TRUE;
		     			break;
		 		} 
			}

			if(IsCondition == FALSE) {	/* Not conditional */
				if(endtoken <= tc) printf("%.6g ",retval.val.d);
			}
		}
 
		count=endtoken;
	}

	sigsetjmp(savestate,1);		/* save current context */
}

printf("\n");

SetLastError(0);
return(0);
}

/*
 * Import statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *filename[MAX_SIZE];
char *extptr;
char *modpath;
char *module_directories[MAX_SIZE][MAX_SIZE];
int pathtc;
int count;
char *fileext[10];

if((char) *tokens[1] == '"') {			/* importing using filename */
	if(IsValidString(tokens[1]) == FALSE) {
		SetLastError(SYNTAX_ERROR);
		return(-1);
	}

	StripQuotesFromString(tokens[1],filename);		/* remove quotes from filename */

	extptr=strrchr(filename,'.');		/* get extension */
	if(extptr == NULL) {
		SetLastError(SYNTAX_ERROR);	/* is valid string */
		return(-1);
	}

	if(memcmp(extptr,".xsc",3) == 0) {	/* is source file */
		if(IncludeFile(filename) == -1) return(-1);	/* including source file */

		SetLastError(0);
		return(0);
	}
}

/* is including from XSCRIPT_MODULE_PATH otherwise */

modpath=getenv("XSCRIPT_MODULE_PATH");				/* get module path */
if(modpath == NULL) {				/* no module path */
	SetLastError(NO_MODULE_PATH);
	return(-1);
}

pathtc=TokenizeLine(modpath,module_directories,":");			/* tokenize line */

for(count=0;count<pathtc;count++) {			/* loop through path array */

	/* try source file first, then binary module */

	sprintf(filename,"%s/%s.xsc",module_directories[count],tokens[1]);	/* get path to source file */

	if(IncludeFile(filename) == 0) {	/* including source file */
		SetLastError(0);
		return(0);
	}
}

SetLastError(MODULE_NOT_FOUND);
return(-1);
}
/*
 * Libcall statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

//LIBCALL function name(arguments) IN module name TO result_variable

int libcall_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int paramendpos;
char *parameters[MAX_SIZE][MAX_SIZE];
int paramcount=0;
int vartype;
int returntype;
UserDefinedType *udtptr;
char *modname[MAX_SIZE];
varsplit split;

if(strcmp(tokens[2],"(") != 0) {
	SetLastError(SYNTAX_ERROR);
	return(-1);
}

/* get parameters*/

for(paramendpos=3;paramendpos<tc;paramendpos++) {		/* find ) */
	if(strcmpi(tokens[paramendpos],")") == 0) break;

	if(strcmpi(tokens[paramendpos],",") == 0) continue; /* skip , */

	strcpy(parameters[paramcount++],tokens[paramendpos]);	/* get parameter */
}

//LIBCALL function name(arguments) IN module name TO result_variable

if((strcmpi(tokens[paramendpos+1],"IN") != 0) || (strcmpi(tokens[paramendpos+3],"TO") != 0)) { 	/* missing IN or TO */
	SetLastError(SYNTAX_ERROR);
	return(-1);
}

StripQuotesFromString(tokens[paramendpos+2],modname);	/* remove quotes from module name */

/* call binary module function */

//int CallModule(char *functionname,char *modulename,int paramcount,varsplit *result,char *parameters[MAX_SIZE][MAX_SIZE]) {

ParseVariableName(tokens,paramendpos+4,tc,&split);		/* split variable name */

return(CallModule(tokens[1],modname,paramcount,&split,parameters));
}

/*
 * If statement
 *
 * In: tc Token count
 * tokens Token array
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
int save_expr_true;
bool has_else_statement=FALSE;

if(tc < 2) {
	SetLastError(SYNTAX_ERROR);					/* Too few parameters */
	return(-1);
}

SetFunctionFlags(IF_STATEMENT);

save_expr_true=-1;

while(*CurrentBufferPosition != 0) {

	if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {  
		if(IsValidExpression(tokens,1,tc-1) == FALSE) {	/* invalid expression */
			SetLastError(INVALID_EXPRESSION);
			return(-1);
		}

		sigsetjmp(savestate,1);		/* save current context */

		exprtrue=EvaluateCondition(tokens,1,tc-1);	/* evaluate condition */
		if(exprtrue == -1) return(-1);

		save_expr_true=exprtrue;			/* save evaluate status */
	}

	if(*CurrentBufferPosition == 0) {
		SetLastError(0);
		return(0);
	}

	if((exprtrue == 1) || ((save_expr_true == 0) && (has_else_statement == TRUE))) {
		save_expr_true=exprtrue;

		if(ExecuteLine(buf) == -1) return(-1);
	}
	
	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	memset(tokens,0,MAX_SIZE*MAX_SIZE);

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
	else if(strcmpi(tokens[0],"ELSE") == 0) { 			
		has_else_statement=TRUE;			/* has ELSE statement */
	}

}

SetLastError(IF_WITHOUT_ENDIF);
return(-1);
}

/*
 * Endif statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number
 *
 */

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) == 0) {
	SetLastError(ENDIF_WITHOUT_IF);
	return(-1);
}

SetLastError(0);
return(0);
}

/*
 * For statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */
int for_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int StartOfFirstExpression;
int StartOfSecondExpression;
int StartOfStepExpression;
double StepValue;
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


if(ParseVariableName(tokens,1,StartOfFirstExpression,&split) == -1) return(-1);

for(StartOfStepExpression=1;StartOfStepExpression<tc;StartOfStepExpression++) {
	if(strcmpi(tokens[StartOfStepExpression],"STEP") == 0) {		/* found start of step expression */
		StartOfStepExpression++;
		break;
	}
}

if(StartOfStepExpression == tc) {		/* no step keyword */
	StepValue=1;
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

	StepValue=EvaluateExpression(outtokens,0,returnvalue);		/* evaulate for step expression */
}

//  0   1    2 3 4  5
// for count = 1 to 10

/* validate start and end values */

if(IsValidExpression(tokens,StartOfFirstExpression,StartOfSecondExpression-1) == FALSE) {
	SetLastError(INVALID_EXPRESSION);
	return(-1);
}

if(IsValidExpression(tokens,StartOfSecondExpression,StartOfStepExpression-1) == FALSE) {
	SetLastError(INVALID_EXPRESSION);
	return(-1);
}

returnvalue=SubstituteVariables(StartOfFirstExpression,StartOfSecondExpression,tokens,outtokens);
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
	CreateVariable(split.name,"DOUBLE",1,1);
}

vartype=GetVariableType(split.name);			/* check if string */

if(vartype == -1) {
	loopcount.d=exprone;
	vartype=VAR_NUMBER;
}
else if(vartype == VAR_STRING) {
	SetLastError(TYPE_ERROR);
	return(-1);
}
else if(vartype == VAR_NUMBER) {
	loopcount.d=exprone;
}
else if(vartype == VAR_INTEGER) {
	loopcount.i=exprone;
}
else if(vartype == VAR_SINGLE) {
	loopcount.f=exprone;
}

//printf("split.name=%lX %s\n",split.name,split.name);
//printf("split.fieldname=%lX %s\n",split.fieldname,split.fieldname);
//printf("split.x=%d\n",split.x);
//printf("split.y=%d\n",split.y);
//printf("split.fieldx=%d\n",split.fieldx);
//printf("split.fieldy=%d\n",split.fieldy);

//asm("int $3");
UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y,split.fieldx,split.fieldy);			/* set loop variable to next */	

//printf("for_statement() 1\n");
//asm("int $3");

if(exprone >= exprtwo) {
	ifexpr=1;
}
else
{
	ifexpr=0;
}

SetFunctionFlags(FOR_STATEMENT);

SetSaveInformationBufferPointer(CurrentBufferPosition);

while(1) {
	sigsetjmp(savestate,1);		/* save current context */

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */	

	RemoveNewline(buf);

	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

	if(strcmpi(tokens[0],"NEXT") != 0) {  
		if(ExecuteLine(buf) == -1) {	/* run statement */
			PopSaveInformation();

			ClearIsRunningFlag();
			return(-1);
		}
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

	sigsetjmp(savestate,1);

	if(strcmpi(tokens[0],"NEXT") == 0) {   
		sigsetjmp(savestate,1);		/* save current context */
  
		SetCurrentFunctionLine(GetSaveInformationLineCount());
		lc=GetSaveInformationLineCount();

		sigsetjmp(savestate,1);		/* save current context */
 
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 0)) loopcount.d += StepValue;      
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 0)) loopcount.i += StepValue;
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 0)) loopcount.f += StepValue;      

		///printf("split.fieldname=%s\n",split.fieldname);

	      	UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y,split.fieldx,split.fieldy);	/* set loop variable to next */	

		//printf("for_statement() 2\n");
		//asm("int $3");

		sigsetjmp(savestate,1);		/* save current context */

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

PopSaveInformation();
//CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* skip over "next" statement */	

SetLastError(NO_ERROR);
return(0);
}

/*
 * Return statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;
int vartype;
int substreturnvalue=0;
char *outtokens[MAX_SIZE][MAX_SIZE];

ClearFunctionFlags(FUNCTION_STATEMENT);

/* check return type */

for(count=1;count<tc;count++) {
	if((IsNumber(tokens[count]) == TRUE) && (GetFunctionReturnType() == VAR_STRING)) {
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
	//if(IsValidExpression(tokens,1,tc) == FALSE) {   /* invalid expression */
	//	retval.has_returned_value=FALSE;	/* clear has returned value flag */

	//	SetLastError(INVALID_EXPRESSION);
	//	return(-1);	
	//}
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
	
	 if(IsValidExpression(tokens,1,tc) == FALSE) {
		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
		return(-1);
	}
	
	retval.val.i=EvaluateExpression(outtokens,0,substreturnvalue+1);
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

SetLastError(0);
return(0);
}

int get_return_value(varval *val) {
	val=&retval.val;
	SetLastError(0);
	return(0);
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & WHILE_STATEMENT) == 0) {
	SetLastError(WEND_NOWHILE);
	return(-1);
}

SetLastError(0);
return(0);
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & FOR_STATEMENT) == 0) {
	SetLastError(NEXT_WITHOUT_FOR);
	return(-1);
}

SetLastError(0);
return(0);
}

/*
 * While statement
 *
 * In: tc Token count
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int exprtrue;
int count;
char *condition_tokens[MAX_SIZE][MAX_SIZE];
int condition_tc;
int returnvalue;
int copycount;

if(tc < 1) {			/* Too few parameters */
	SetLastError(SYNTAX_ERROR);
	return(-1);
}

count=0;

for(copycount=1;copycount<tc;copycount++) {
	strcpy(condition_tokens[count++],tokens[copycount]);
}

condition_tc=count;

SetFunctionFlags(WHILE_STATEMENT);

PushSaveInformation();		/* save while loop state */

SetSaveInformationBufferPointer(CurrentBufferPosition);
	
do {
	sigsetjmp(savestate,1);		/* save current context */

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	printf("buf=%s\n",buf);

	RemoveNewline(buf);

	if(IsValidExpression(condition_tokens,0,condition_tc) == FALSE) {
		PopSaveInformation();

		SetLastError(INVALID_EXPRESSION);	/* invalid expression */
		return(-1);
	}

	exprtrue=EvaluateCondition(condition_tokens,0,condition_tc);			/* do condition */

	sigsetjmp(savestate,1);		/* save current context */

	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) {	
		PopSaveInformation();
		SetLastError(SYNTAX_ERROR);
		return(-1);
	}

	if(strcmpi(tokens[0],"WEND") == 0) {
		if(exprtrue == 1) {				/* at end of block, go back to start */
		    	SetCurrentFunctionLine(GetSaveInformationLineCount());
		
			CurrentBufferPosition=GetSaveInformationBufferPointer();
		}
		else
		{
			break;
		}
	}

	returnvalue=ExecuteLine(buf);

	if(returnvalue != 0) {
		PopSaveInformation();
		ClearIsRunningFlag();

		return(returnvalue);
	}

	if(GetIsRunningFlag() == FALSE) return(NO_ERROR);	/* program ended */
} while(exprtrue == 1);

PopSaveInformation();

return(0);			 
}

/*
 * End statement
 *
 * In: tc Token count
 *     tokens Token array
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
 *     tokens Token array
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
	tokens	Token array
 *
 */

int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) {
	SetLastError(ELSE_WITHOUT_IF);
	return(-1);
}

SetLastError(0);
return(0);
}

/*
 * Endfunction statement
 *
 * In: tc Token count
 * tokens Token array
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
 * tokens Token array
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
 * tokens Token array
 *
 * Returns error number on error or 0 on success
 *
 */

int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
varsplit split;
int vartype;
int count;

/* if there is a type in the declare statement */

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"AS") == 0) break;
}

if(count < tc) {		/* found type */
	if(ParseVariableName(tokens,1,count-1,&split) == -1) return(-1);	/* parse variable name */

	retval.val.type=vartype;
	retval.val.i=CreateVariable(split.name,tokens[count+1],split.x,split.y);
}
else
{
	if(ParseVariableName(tokens,1,tc,&split) == -1) return(-1);	/* parse variable name */

	retval.val.type=VAR_NUMBER;
	retval.val.i=CreateVariable(split.name,vartypenames[DEFAULT_TYPE_INT],split.x,split.y);
}

if(retval.val.i == -1) return(-1);

SetLastError(0);
return(0);
}

/*
 * Iterate statement
 *
 * In: tc Token count
 * tokens Token array
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

SetLastError(ITERATE_WITHOUT_LOOP);
return(-1);
}

/*
 * Type statement
 *
 * In: tc Token count
 *	tokens Token array
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
	
	if(ParseVariableName(typetokens,0,typetc,&split) == -1) return(-1);		/* split variable name */

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
 * tokens Token array
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
 * tokens Token array
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
 * tokens Token array
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
 * tokens Token array
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
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}
if((var->xsize == 0) && (var->ysize == 0)) {
	SetLastError(NOT_ARRAY);
	return(-1);
}

if(ParseVariableName(tokens,1,tc,&split) == -1) return(-1);		/* parse variable name */

SetLastError(ResizeArray(split.name,split.x,split.y));
return(-1);
}
/*
 * Help command
 *
 * In: tc Token count
 *	tokens Token array
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
 * tokens Token array
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
 * In: linebuf				Line to tokenize
       tokens[MAX_SIZE][MAX_SIZE]	Token array output
 *
 * Returns -1 on error or token count on success
 *
 */

/* TODO: Rewrite this function */

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split) {
char *token;
int tc;
int count;
char *d;
char *s;
int IsSeperatorCharacter;
char *lastcharptr;
char *nextcharptr;

token=linebuf;

while(((char) *token == ' ') || ((char) *token == '\t')) token++;	/* skip leading whitespace characters */

/* tokenize line */

	tc=0;
	
	d=tokens[0];
	memset(d,0,MAX_SIZE);				/* clear line */
	
	while((char) *token != 0) {
		IsSeperatorCharacter=FALSE;

		if(*token == '"' ) {		/* quoted text */ 
	   		*d++=*token++;

	   		while((char)  *token != 0) {
	    			*d++=*token++;

	    			if(*(d-1) == '"') break;		/* quoted text */	

	  		}

	  		tc++;
	 	}
	 	else
	 	{
	  		s=split;

	  		while(*s != 0) {
	    			if((char) *token == *s) {		/* seperator found */

			    		if((strlen(tokens[tc]) != 0) && (*token != '.')) tc++;
	    
			    		IsSeperatorCharacter=TRUE;
			    		d=tokens[tc]; 			

			    		memset(d,0,MAX_SIZE);				/* clear line */

					/* ignore spaces between tokens; this then allows the following be be equivalent:
						2 + 2
						2+2
					*/

			    		if((char) *token != ' ') {

						/* handle seperators that are multi-character operators */

						if((char) *token == '*') {		/* ** */
							nextcharptr=token;
							nextcharptr++;
		
							if((char) *nextcharptr != '*') {
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '>') {		/* >> or >= */
							nextcharptr=token;
							nextcharptr++;
										
							if((char) *nextcharptr != '=') {
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];
						}
						else if((char) *token == '<') {		/* << or <= */					
							nextcharptr=token;
							nextcharptr++;
	
							if((char) *nextcharptr != '=') {
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '!') {	
							nextcharptr=token;
							nextcharptr++;
										
							if((char) *nextcharptr != '=') {
								tc++;
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '.') {		/* can be either decimal point or member */
							lastcharptr=token;
							lastcharptr--;

							nextcharptr=token;
							nextcharptr++;

							if( (((char)  *lastcharptr >= '0') && ((char)  *lastcharptr <= '9')) && 
							    (((char) *nextcharptr >= '0') && ((char) *nextcharptr <= '9'))) {
								*d++=*lastcharptr;
								*d++=*token++;
								*d++=*token++;
							}
							else
							{
								tc++;
							}
						}
						else if((char) *token == '-') {		/* can be either part of the
											token as a negative sign or a subtract operator token */
							nextcharptr=token;
							nextcharptr++;

							if((char) *nextcharptr == ' ') tc++;
	
				      			*d++=*token++;
						}
						else		/* single character seperator */
						{
			      				*d++=*token++;	
				     		     	d=tokens[++tc];
						}
			    		}
			    		else
					{
						token++;		/* ignore spaces next to tokens */
					}
	   	}

	 	s++;
	 }

	 if(IsSeperatorCharacter == FALSE) *d++=*token++; /* non-token character */
	}
}

if(strlen(tokens[tc]) > 0) tc++;		/* if there is data in the last token, increment the counter so it is accounted for */

//for(int countz=0;countz < tc;countz++) {
//	printf("token[%d]=%s\n",countz,tokens[countz]);
//}

return(tc);
}

/*
 * Check if seperator
 *
 * In: token		Token to check
       sep		Seperator characters to check against
 *
 * Returns TRUE or FALSE
 *
 */
int IsSeperator(char *token,char *sep) {
char *SepPtr;

if(*token == 0) return(TRUE);
	
SepPtr=sep;

while(*SepPtr != 0) {
	if((char) *SepPtr++ == (char) *token) return(TRUE);
}

if(IsStatement(token) == TRUE) return(TRUE);

return(FALSE);
}
	 
/*
 * Convert to uppercase
 *
 * In: char *token	String to convert
 *
 * Returns: nothing
 *
 */

void ToUpperCase(char *token) {
	char *tokenptr;
	
	tokenptr=token;

	while(*tokenptr != 0) { 	/* until end */
		if(((char) *tokenptr >= 'a') &&  ((char) *tokenptr <= 'z')) *tokenptr -= 32;	/* convert to lower case if upper case */
	  	tokenptr++;
	}
}

/*
 * Read line from buffer
 *
 * In:	buf		Buffer to read from
	linebuf		Buffer to store line
	size		Maximum size of line
 *
 * Returns -1 on error or address of next address in buffer for success
 *
 */

char *ReadLineFromBuffer(char *buf,char *linebuf,int size) {
int count=0;
char *lineptr;

memset(linebuf,0,size);

lineptr=linebuf;

do {
	if(count++ == size) break;

	*lineptr++=*buf;

	if( ((char) *buf) == '\n' || ((char) *buf) == '\r') {
		lineptr--;
		*lineptr=0;
		break;
	}

} while(((char) *buf++) != 0);		/* until end of line */

return(++buf);			/* return new position */
}

/*
 * Compare string case insensitively
 *
 * In: source		First string
       dest		Second string
 *
 * Returns: 0 if matches, positive or negative number otherwise
 *
 */
int strcmpi(char *source,char *dest) {
char *sourcetemp[MAX_SIZE];
char *desttemp[MAX_SIZE];

/* create copies of the string and convert them to uppercase */

memset(sourcetemp,0,MAX_SIZE);
memset(desttemp,0,MAX_SIZE);

strcpy(sourcetemp,source);
strcpy(desttemp,dest);

ToUpperCase(sourcetemp);
ToUpperCase(desttemp);

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
FILE *handle;
MODULES ModuleEntry;
char *bufpos;
int returnvalue;
char *saveCurrentBufferPosition;
char *linebuf[MAX_SIZE];

if(stat(filename,&includestat) == -1) {
	SetLastError(FILE_NOT_FOUND);
	return(-1);
}

handle=fopen(filename,"rb");				/* open module file */
if(!handle) {
	SetLastError(FILE_NOT_FOUND);
	return(-1);
}

/* read module into buffer */

ModuleEntry.StartInBuffer=malloc(includestat.st_size);		/* start adress of module in buffer */
if(ModuleEntry.StartInBuffer == NULL) {
	fclose(handle);

	SetLastError(NO_MEM);
	return(-1);
}

if(fread(ModuleEntry.StartInBuffer,includestat.st_size,1,handle) != 1) {
	free(ModuleEntry.StartInBuffer);
	fclose(handle);

	SetLastError(IO_ERROR);
	return(-1);
}

/* put null at end of buffer */

bufpos=(ModuleEntry.StartInBuffer+(includestat.st_size-1));
*bufpos=0;

fclose(handle);

/* add module entry */
strcpy(ModuleEntry.modulename,filename);	/* module filename */
ModuleEntry.flags=MODULE_SCRIPT;		/* module type */
ModuleEntry.EndInBuffer=ModuleEntry.StartInBuffer;			/* end adress of module in buffer */
ModuleEntry.EndInBuffer += includestat.st_size;
AddToModulesList(&ModuleEntry);

returnvalue=0;

saveCurrentBufferPosition=GetCurrentBufferPosition();		/* save current pointer */
SetCurrentFileBufferPosition(ModuleEntry.StartInBuffer);	/* set start */
CurrentBufferPosition=ModuleEntry.StartInBuffer;

do {

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,linebuf,LINE_SIZE);	/* read line from buffer */

	if(((char) *CurrentBufferPosition) == 0) break;

	if(ExecuteLine(linebuf) == -1) return(-1);		/* run statement */

	memset(linebuf,0,MAX_SIZE);

} while(*CurrentBufferPosition != 0); 			/* until end */

CurrentBufferPosition=saveCurrentBufferPosition;	/* restore current pointer */
SetCurrentFileBufferPosition(CurrentBufferPosition);

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

void RemoveNewline(char *line) {
char *b;

if(strlen(line) > 1) {
	b=(line+strlen(line))-1;
	if((*b == '\n') || (*b == '\r')) *b=0;
}
}

