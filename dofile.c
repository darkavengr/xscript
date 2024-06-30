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

extern jmp_buf savestate;

int saveexprtrue=0;
functionreturnvalue retval;
char *CurrentBufferPosition=NULL;	/* current pointer in buffer - points to either FileBufferPosition or interactive mode buffer */
char *FileBufferPosition=NULL;
char *endptr=NULL;		/* end of buffer */
char *FileBuffer=NULL;		/* buffer */
int FileBufferSize=0;		/* size of buffer */
int NumberOfIncludedFiles=0;	/* number of included files */
char *TokenCharacters="+-*/<>=!%~|& \t()[],{};.";
int Flags=0;
char *CurrentFile[MAX_SIZE];

/*
 * Load file
 *
 * In: char *filename		Filename of file to load
 *
 * Returns error number on error or 0 on success
 *
 */
int LoadFile(char *filename) {
FILE *handle; 
int filesize;

handle=fopen(filename,"r");				/* open file */
if(!handle) return(FILE_NOT_FOUND);

fseek(handle,0,SEEK_END);				/* get file size */
filesize=ftell(handle);
fseek(handle,0,SEEK_SET);				/* seek back to start */
	
if(FileBuffer == NULL) {				/* first time */
	FileBuffer=malloc(filesize+1);			/* allocate buffer */

	if(FileBuffer == NULL) return(NO_MEM);		/* no memory */
}
else
{
	if(realloc(FileBuffer,FileBufferSize+filesize) == NULL) return(NO_MEM);		/* resize buffer */
}

FileBufferPosition=FileBuffer;

if(fread(FileBuffer,filesize,1,handle) != 1) return(READ_ERROR);		/* read to buffer */
		
endptr=(FileBuffer+filesize);		/* point to end */
*endptr=0;			/* put null at end */

FileBufferSize += filesize;

strcpy(CurrentFile,filename);

SetIsFileLoadedFlag();
ClearIsRunningFlag();
return(0);
}

/*
 * Load and execute file
 *
 * In: char *filename		Filename of file to load
 *
 * Returns -1 on error or 0 on success
 *
 */
int ExecuteFile(char *filename) {
char *linebuf[MAX_SIZE];
char *saveCurrentBufferPosition;
int lc=1;
int returnvalue;

SetCurrentFunctionLine(lc);		/* set line number to 1 */
	
if(LoadFile(filename) > 0) return(FILE_NOT_FOUND);

SetIsRunningFlag();
SetIsFileLoadedFlag();

SwitchToFileBuffer();			/* switch to file buffer */

saveCurrentBufferPosition=CurrentBufferPosition;		/* save current pointer */
CurrentBufferPosition=FileBuffer;

SetFunctionCallPtr(CurrentBufferPosition);		/* set start of current function to buffer start */

do {
	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,linebuf,LINE_SIZE);			/* get data */

	returnvalue=ExecuteLine(linebuf);			/* run statement */

	if(returnvalue != 0) {
		ClearIsRunningFlag();
		return(returnvalue);
	}

	if(GetIsRunningFlag() == FALSE) {
		CurrentBufferPosition=saveCurrentBufferPosition;
		return(NO_ERROR);	/* program ended */
	}

	memset(linebuf,0,MAX_SIZE);

	lc=GetCurrentFunctionLine();		/* increment line number */
	SetCurrentFunctionLine(++lc);

}    while(*CurrentBufferPosition != 0); 			/* until end */

CurrentBufferPosition=saveCurrentBufferPosition;

return(NO_ERROR);
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

c=*lbuf;
if((c == '\r') || (c == '\n') || (c == 0)) return(0);			/* blank line */

if(GetTraceFlag()) {		/* print statement if trace is enabled */
	GetCurrentFunctionName(functionname);

	printf("***** Tracing line %d in function %s: %s\n",GetCurrentFunctionLine(),functionname,lbuf);
}

if(strlen(lbuf) > 1) {
	b=lbuf+strlen(lbuf)-1;
	if((*b == '\n') || (*b == '\r')) *b=0;
}

while(*lbuf == ' ' || *lbuf == '\t') lbuf++;	/* skip white space */

if(memcmp(lbuf,"//",2) == 0) return(0);		/* skip comments */

memset(tokens,0,MAX_SIZE*MAX_SIZE);

tc=TokenizeLine(lbuf,tokens,TokenCharacters);			/* tokenize line */

//if(CheckSyntax(tokens,TokenCharacters,1,tc) == 0) return(SYNTAX_ERROR);		/* check syntax */

if(IsStatement(tokens[0])) {			/* run statement if statement */
	return(CallIfStatement(tc,tokens));
}


/*
 *
 * assignment
 *
 */

for(count=1;count<tc;count++) {

	if(strcmpi(tokens[count],"=") == 0) {
 		ParseVariableName(tokens,0,count-1,&split);			/* split variable */  	

		SubstituteVariables(count+1,tc,tokens,tokens);
 		if(returnvalue > 0) return(returnvalue);
		
	 	if(strlen(split.fieldname) == 0) {			/* use variable name */
	 		vartype=GetVariableType(split.name);
		}
		else
		{
	 		vartype=GetFieldTypeFromUserDefinedType(split.name,split.fieldname);
			if(vartype == -1) return(TYPE_FIELD_DOES_NOT_EXIST);
		}

		c=*tokens[count+1];

		if((c != '"') && (vartype == VAR_STRING)) return(TYPE_ERROR);

		if( ((c == '"') && (vartype == VAR_STRING))) {			/* string */  
			if(vartype == -1) CreateVariable(split.name,"STRING",split.x,split.y);		/* new variable */ 
		  		 
		  	ConatecateStrings(count+1,tc,tokens,&val);					/* join all the strings on the line */

		  	UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);		/* set variable */
		  	return(0);
		}

		/* number otherwise */

		 if( ((c == '"') && (vartype != VAR_STRING))) return(TYPE_ERROR);
	
		 if(IsValidExpression(tokens,count+1,tc) == FALSE) return(INVALID_EXPRESSION);	/* invalid expression */
	
		 exprone=EvaluateExpression(tokens,count+1,tc);

		 if(vartype == VAR_NUMBER) {
	 		val.d=exprone;
	 	 }
		 else if(vartype == VAR_STRING) {
			returnvalue=SubstituteVariables(count+1,count+1,tokens,tokens);
 			if(returnvalue > 0) return(returnvalue);

		 	strcpy(val.s,tokens[count+1]);  
		}
		else if(vartype == VAR_INTEGER) {
	 		val.i=exprone;
	 	}
	 	else if(vartype == VAR_SINGLE) {
	 		val.f=exprone;
	 	}
	 	else if(vartype == VAR_UDT) {			/* user-defined type */	 
			ParseVariableName(tokens,count+1,tc,&assignsplit);		/* split variable */  	
 			varptr=GetVariablePointer(split.name);		/* point to variable entry */
			assignvarptr=GetVariablePointer(assignsplit.name);

			if((varptr == NULL) || (assignvarptr == NULL)) return(VARIABLE_DOES_NOT_EXIST);
			   
			CopyUDT(assignvarptr->udt,assignvarptr->udt);		/* copy UDT */

			return(0);
 		}

		if(vartype == -1) {		/* new variable */ 	  
		 	val.d=exprone;
		 	CreateVariable(split.name,"DOUBLE",split.x,split.y);			/* create variable */
		 	UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);
	
		 	return(0);
		 }

		 UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);
		 return(0);
	 }

}

/* call user function */

if(CheckFunctionExists(tokens[0]) != -1) {	/* user function */
	return(CallFunction(tokens,0,tc));
} 

return(INVALID_STATEMENT);
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

DeclareFunction(&tokens[1],tc-1);

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

for(count=1;count<tc;count++) {
	/* if string literal, string variable or function returning string */

	for(endtoken=count+1;endtoken<tc;endtoken++) {
		if(strcmp(tokens[endtoken],",") == 0) break;
	}

	if(((char) *tokens[count] == '"') || (GetVariableType(tokens[count]) == VAR_STRING) || (CheckFunctionExists(tokens[count]) == VAR_STRING) ) {

		memset(printtokens,0,MAX_SIZE*MAX_SIZE);

		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);	
		if(returnvalue > 0) return(returnvalue);		/* error occurred */

		count += ConatecateStrings(count,endtoken,printtokens,&val);		/* join all the strings in the token */

		printf("%s ",val.s);
	}
	else
	{
		memset(printtokens,0,MAX_SIZE*MAX_SIZE);

		returnvalue=SubstituteVariables(count,endtoken,tokens,printtokens);
		if(returnvalue > 0) return(returnvalue);		/* error occurred */

		retval.val.type=0;

		if(strlen(tokens[count]) > 0) {		/* if there are tokens substituted */
			retval.val.d=EvaluateExpression(printtokens,count,endtoken);

			/* if it's a condition print True or False */

			for(countx=count;countx<tc;countx++) {
				if((strcmp(tokens[countx],">") == 0) || (strcmp(tokens[countx],"<") == 0) || (strcmp(tokens[countx],"=") == 0)) {

					retval.val.type=0;
		     			retval.val.d=EvaluateCondition(tokens,count,endtoken);

		     			retval.val.d == 1 ? printf("True ") : printf("False ");
		     			break;
		 		} 
			}

			if(countx == tc) printf("%.6g ",retval.val.d);	/* Not conditional */
		}
	}

	count=endtoken;
}

printf("\n");

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
	if(IsValidString(tokens[1]) == FALSE) return(SYNTAX_ERROR);	/* is valid string */

	StripQuotesFromString(tokens[1],filename);		/* remove quotes from filename */

	return(IncludeFile(filename));	/* including source file */
}

return(AddModule(tokens[1]));		/* load module */
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

if(tc < 2) return(SYNTAX_ERROR);					/* Too few parameters */

SetFunctionFlags(IF_STATEMENT);

while(*CurrentBufferPosition != 0) {

	if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {  
		exprtrue=EvaluateCondition(tokens,1,tc);

		if(exprtrue == -1) return;

		if(exprtrue == 1) {
			saveexprtrue=exprtrue;

			do {
		   		CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) return(0);

				returnvalue=ExecuteLine(buf);
				if(returnvalue > 0) return(returnvalue);

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) return(SYNTAX_ERROR);
				
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					SetFunctionFlags(IF_STATEMENT);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
	 	}
	 	else
	 	{
			do {
	   			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) return(0);
	

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) return(SYNTAX_ERROR);
				
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					SetFunctionFlags(IF_STATEMENT);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
		}
	}


	if((strcmpi(tokens[0],"ELSE") == 0)) {

		if(saveexprtrue == 0) {
	    		do {
	   			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				if(*CurrentBufferPosition == 0) return(0);

				returnvalue=ExecuteLine(buf);
				if(returnvalue > 0) return(returnvalue);


				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

				if(tc == -1) return(SYNTAX_ERROR);	
				
				if(strcmpi(tokens[0],"ENDIF") == 0) {
					ClearFunctionFlags(IF_STATEMENT);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
		}
	}

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
}

return(ENDIF_NOIF);
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
if((GetFunctionFlags() & IF_STATEMENT) == 0) return(ENDIF_NOIF);

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
int count;
int countx;
int steppos;
double exprone;
double exprtwo;
varval loopcount;
varval loopx;
int ifexpr;
char *d;
char *buf[MAX_SIZE];
int vartype;
varsplit split;

int returnvalue;
int lc;

SetFunctionFlags(FOR_STATEMENT);

/* find end of variable name */

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"=") == 0) break;
}

if(count == tc) {		/* no = */
	ClearFunctionFlags(FOR_STATEMENT);
	return(SYNTAX_ERROR);
}

ParseVariableName(tokens,1,count-1,&split);


//  0  1     2 3 4  5
// for count = 1 to 10

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"TO") == 0) break;		/* found to */
}

if(count == tc) return(SYNTAX_ERROR);

for(countx=1;countx<tc;countx++) {
	if(strcmpi(tokens[countx],"step") == 0) break;		/* found step */
}

if(countx == tc) {
	steppos=1;
}
else
{
	returnvalue=SubstituteVariables(countx+1,tc,tokens,tokens);
	if(returnvalue > 0) return(returnvalue);
	
	if(IsValidExpression(tokens,countx+1,tc) == FALSE) return(INVALID_EXPRESSION);	/* invalid expression */

	steppos=EvaluateExpression(tokens,countx+1,tc);
}

//  0   1    2 3 4  5
// for count = 1 to 10

returnvalue=SubstituteVariables(1,tc,tokens,tokens);
if(returnvalue > 0) return(returnvalue);

if(IsValidExpression(tokens,3,count) == FALSE) return(INVALID_EXPRESSION);	/* invalid expression */
if(IsValidExpression(tokens,count+1,countx) == FALSE) return(INVALID_EXPRESSION);	/* invalid expression */

exprone=EvaluateExpression(tokens,3,count);			/* start value */
exprtwo=EvaluateExpression(tokens,count+1,countx);			/* end value */

if(IsVariable(split.name) == -1) {				/* create variable if it doesn't exist */
	CreateVariable(split.name,"DOUBLE",split.x,split.y);
}

vartype=GetVariableType(split.name);			/* check if string */

switch(vartype) {
	case -1:
		loopcount.d=exprone;
		vartype=VAR_NUMBER;
		break;

	case VAR_STRING: 
		return(TYPE_ERROR);

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

PushSaveInformation();					/* save line information */

CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */	
	
do {	  
	returnvalue=ExecuteLine(buf);
	if(returnvalue != 0) {
		ClearIsRunningFlag();
		return(returnvalue);
	}

	if(GetBreakFlag() == TRUE) {		/* end of loop */
		ClearBreakFlag();
		return(0);
	}

	if(GetIsRunningFlag() == FALSE) return(NO_ERROR);	/* program halted */
		    
	d=*buf+(strlen(buf)-1);
	if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
	if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

	lc=GetCurrentFunctionLine();		/* increment line number */
	SetCurrentFunctionLine(++lc);

 	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	
	if(strcmpi(tokens[0],"NEXT") == 0) {     	
		SetCurrentFunctionLine(GetSaveInformationLineCount());

		CurrentBufferPosition=GetSaveInformationBufferPointer();			/* get pointer to start of for statement */

		SetCurrentFunctionLine(GetSaveInformation()->linenumber);		/* return line counter to start */

	      /* increment or decrement counter */
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 1)) loopcount.d -= steppos;
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 0)) loopcount.d += steppos;      
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 1)) loopcount.i -= steppos;
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 0)) loopcount.i += steppos;      
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 1)) loopcount.f -=steppos;
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 0)) loopcount.f += steppos;      

	      	UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y);			/* set loop variable to next */	

	      	if(*CurrentBufferPosition == 0) {
	      		PopSaveInformation();				 
			ClearFunctionFlags(FOR_STATEMENT);

			return(SYNTAX_ERROR);
	     	 }
	    	} 

		CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */	
} while(
       ( (vartype == VAR_NUMBER) && (ifexpr == 1) && (loopcount.d >= exprtwo)) ||
       ((vartype == VAR_NUMBER) && (ifexpr == 0) && (loopcount.d <= exprtwo)) ||
       ((vartype == VAR_INTEGER) && (ifexpr == 1) && (loopcount.i >= exprtwo)) ||
       ((vartype == VAR_INTEGER) && (ifexpr == 0) && (loopcount.i <= exprtwo)) ||
       ((vartype == VAR_SINGLE) && (ifexpr == 1) && (loopcount.f >= exprtwo)) ||
       ((vartype == VAR_SINGLE) && (ifexpr == 0) && (loopcount.f <= exprtwo))
       );

return(NO_ERROR);	
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

SetFunctionFlags(FUNCTION_STATEMENT);

/* check return type */

for(count=1;count<tc;count++) {
	if((*tokens[count] >= '0' && *tokens[count] <= '9') && (GetFunctionReturnType() == VAR_STRING)) return(TYPE_ERROR);

 	if((*tokens[count] == '"') && (GetFunctionReturnType() != VAR_STRING)) return(TYPE_ERROR);

 	vartype=GetVariableType(tokens[count]);

 	if(vartype != -1) {
		if((GetFunctionReturnType() == VAR_STRING) && (vartype != VAR_STRING)) return(TYPE_ERROR);

  		if((GetFunctionReturnType() != VAR_STRING) && (vartype == VAR_STRING)) return(TYPE_ERROR);
	}
}

retval.val.type=GetFunctionReturnType();		/* get return type */

retval.has_returned_value=TRUE;				/* set has returned value flag */

if(GetFunctionReturnType() != VAR_STRING) {		/* returning number */

	if(IsValidExpression(tokens,2,tc) == FALSE) {   /* invalid expression */
		retval.has_returned_value=FALSE;	/* clear has returned value flag */

		return(INVALID_EXPRESSION);	
	}
}

if(GetFunctionReturnType() == VAR_STRING) {		/* returning string */
	ConatecateStrings(1,tc,tokens,&retval.val);		/* get strings */
}
else if(GetFunctionReturnType() == VAR_INTEGER) {		/* returning integer */
	substreturnvalue=SubstituteVariables(1,tc,tokens,tokens);
	if(substreturnvalue > 0) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */
		return(substreturnvalue);
	}
	
	retval.val.i=EvaluateExpression(tokens,1,tc);
}
else if(GetFunctionReturnType() == VAR_NUMBER) {		/* returning double */	 
	substreturnvalue=SubstituteVariables(1,tc,tokens,tokens);
	if(substreturnvalue > 0) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */
		return(substreturnvalue);
	}

	 retval.val.d=EvaluateExpression(tokens,1,tc);
}
else if(GetFunctionReturnType() == VAR_SINGLE) {		/* returning single */
	substreturnvalue=SubstituteVariables(1,tc,tokens,tokens);
	if(substreturnvalue > 0) {
		retval.has_returned_value=FALSE;	/* clear has returned value flag */
		return(substreturnvalue);
	}

	retval.val.f=EvaluateExpression(tokens,1,tc);	
}

ReturnFromFunction();			/* return */
return(0);
}

int get_return_value(varval *val) {
	val=&retval.val;
	return(0);
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if((GetFunctionFlags() & WHILE_STATEMENT) == 0) return(WEND_NOWHILE);
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if((GetFunctionFlags() & FOR_STATEMENT) == 0) return(NEXT_WITHOUT_FOR);

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
char *condition_tokens_substituted[MAX_SIZE][MAX_SIZE];
int condition_tc;
int substreturnvalue;

if(tc < 1) return(SYNTAX_ERROR);			/* Too few parameters */

PushSaveInformation();					/* save line information */

memcpy(condition_tokens,tokens,((tc*MAX_SIZE)*MAX_SIZE)/sizeof(tokens));		/* save copy of condition */

condition_tc=tc;

SetFunctionFlags(WHILE_STATEMENT);
	
do {
	     CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	     if(IsValidExpression(tokens,1,condition_tc) == FALSE) return(INVALID_EXPRESSION);	/* invalid expression */

	     exprtrue=EvaluateCondition(condition_tokens,1,condition_tc);			/* do condition */

	     if(exprtrue == 0) {
	     		while(*CurrentBufferPosition != 0) {

	     		  	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) return(SYNTAX_ERROR);
				
				if(strcmpi(tokens[0],"WEND") == 0) {
					PopSaveInformation();				 

					CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
					return(0);
				}
			}

	     }

	     tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
		if(tc == -1) return(SYNTAX_ERROR);
	    
	     if(strcmpi(tokens[0],"WEND") == 0) {
		SetCurrentFunctionLine(GetSaveInformation()->linenumber);		/* return line counter to start */
	     }

	     substreturnvalue=ExecuteLine(buf);

	     if(substreturnvalue != 0) {
	     	ClearIsRunningFlag();
		return(substreturnvalue);
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
 *     tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
/* If in interactive mode, return to command prompt, otherwise exit interpreter */

	if(GetInteractiveModeFlag() == TRUE) {
		ClearIsRunningFlag();

		return(atoi(tokens[1]));
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
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) return(ELSE_WITHOUT_IF);
}

/*
 * Elseif statement
 *
 * In: tc	Token count
		tokens	Tokens array
 *
 */

int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) return(ELSE_WITHOUT_IF);
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
if((GetFunctionFlags() & FUNCTION_STATEMENT) != FUNCTION_STATEMENT) return(ENDFUNCTION_NO_FUNCTION);

ClearFunctionFlags(FUNCTION_STATEMENT);
return(0);
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
	if((GetFunctionFlags() & FOR_STATEMENT) == 0) return(EXIT_FOR_WITHOUT_FOR);
}
else if(strcmpi(tokens[1],"WHILE") == 0) {
	if((GetFunctionFlags() & WHILE_STATEMENT) == 0)	return(EXIT_WHILE_WITHOUT_WHILE);	
}
else
{
	return(SYNTAX_ERROR);
}

/* find end of loop */
while(*CurrentBufferPosition != 0) {
	savebuffer=CurrentBufferPosition;

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,MAX_SIZE);			/* get data */

	TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

	SetBreakFlag();		/* set break flag - will exit from for loop then it returns to the for loop */

	if((strcmpi(tokens[1],"WEND") == 0) && (GetFunctionFlags() & WHILE_STATEMENT)){
	 	ClearFunctionFlags(WHILE_STATEMENT);
		return(0);
	}
   } 

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
	retval.val.i=VAR_INTEGER;
	retval.val.i=CreateVariable(split.name,tokens[count+1],split.x,split.y);
}
else
{
	retval.val.i=VAR_INTEGER;
	retval.val.i=CreateVariable(split.name,"DOUBLE",split.x,split.y);
}

if(retval.val.i != NO_ERROR) return(retval.val.i);

return;
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
	return(0);
}

return(CONTINUE_NO_LOOP);
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

if(tc < 1) return(SYNTAX_ERROR);			/* Too few parameters */

if((GetUDT(tokens[1]) != NULL) || (IsValidVariableType(tokens[1]) != -1)) return(TYPE_EXISTS);		/* If type exists */

/* create user-defined type entry */

strcpy(addudt.name,tokens[1]);

addudt.field=malloc(sizeof(UserDefinedTypeField));	/* add first field */
if(addudt.field == NULL) return(NO_MEM); 
	
fieldptr=addudt.field;

/* add user-defined type fields */
	
do {

	CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

	typetc=TokenizeLine(buf,typetokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) return(SYNTAX_ERROR); 

	if(strcmpi(typetokens[0],"ENDTYPE") == 0) {		/* end of statement */
		AddUserDefinedType(&addudt);			/* add user-defined type */
		return(0);
	}

	if(strcmpi(typetokens[1],"AS") != 0) return(SYNTAX_ERROR); /* missing as */

/* add field to user defined type */
	
	ParseVariableName(typetokens,0,typetc,&split);		/* split variable name */

	strcpy(fieldptr->fieldname,split.name);		/* copy name */
	
	fieldptr->xsize=split.x;				/* get x size of field variable */
	fieldptr->ysize=split.y;				/* get y size of field variable */
	fieldptr->type=IsValidVariableType(typetokens[2]);

/* get type of field variable */

	if(fieldptr->type == -1) return(TYPE_ERROR); 	/* is valid type */

	if(*CurrentBufferPosition == 0) break;		/* at end */
	
/* add link to next field */
	fieldptr->next=malloc(sizeof(UserDefinedTypeField));
	if(fieldptr->next == NULL) return(NO_MEM); 
	
	fieldptr=fieldptr->next;

} while(*CurrentBufferPosition != 0);

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
			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

			tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */
	
			if(strcmpi(trytokens[0],"ENDTRY") == 0) return(0);
		}

		return(TRY_WITHOUT_ENDTRY);
	}


	if(ExecuteLine(buf) > 0) {			/* run statement */
		/* error occurred */

		while(*CurrentBufferPosition != 0) {	/* find catch block */
			CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */

			tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */

			if(strcmpi(trytokens[0],"CATCH") == 0) {	/* found catch block */

			/* run catch statements */

				while(*CurrentBufferPosition != 0) {
					CurrentBufferPosition=ReadLineFromBuffer(CurrentBufferPosition,buf,LINE_SIZE);			/* get data */
					tc=TokenizeLine(buf,trytokens,TokenCharacters);			/* tokenize line */

					if(strcmpi(trytokens[0],"ENDTRY") == 0) return(0);		/* at end of catch block */

					returnvalue=ExecuteLine(buf);
					if(returnvalue > 0) return(returnvalue);		/* run statement and return if error */
				}
			}
		}

		return(TRY_WITHOUT_CATCH);		/* no catch block */
	}
}

return(TRY_WITHOUT_ENDTRY);		/* no endtry statement */
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
return(ENDTRY_WITHOUT_TRY);
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
return(CATCH_WITHOUT_TRY);
}
	
/*
 * Help statement
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

DisplayHelp(dirname,tokens[1]);		/* display help */

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
	return(SYNTAX_ERROR);
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

/* check if brackets are balanced */

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"(") == 0) {
		commacount=0;

		variablenameindex=count-1;			/* save name index */

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

	if(strcmp(tokens[count],",") == 0) {		/* list of expressions */
	//	if(IsInBracket == FALSE) return(FALSE);	/* list not in brackets */

	//	if((IsVariable(tokens[variablenameindex]) == TRUE) && (commacount++ > 2)) return(FALSE); /* too many commas for array */
	}
}


if((bracketcount != 0) || (squarebracketcount != 0)) return(FALSE);

for(count=start;count<end;count += 2) {

/* check if two separators are together */

	if(*tokens[count] == 0) break;

	if((strcmpi(tokens[0],"HELP") != 0) && (strcmpi(tokens[0],"PRINT") != 0)  && (strcmpi(tokens[0],"EXIT") != 0)) {
		if((IsSeperator(tokens[count],separators) == 1) && (IsSeperator(tokens[count+1],separators) == 1)) {
	   
		/* brackets can be next to separators */

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
  		return(SYNTAX_ERROR);
	}
}

return(TRUE);
}

	 
/*
 * Convert to uppercase
 *
 * In: char *token	String to convert
 *
 * Returns -1 on error or 0 on success
 *
 */

int touppercase(char *token) {
	char *z;
	char c;

	z=token;

	while(*z != 0) { 	/* until end */
	 if(*z >= 'a' &&  *z <= 'z') {				/* convert to lower case if upper case */
	  *z -= 32;
	 }

	 z++;
	}

return(0);
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

return(NO_ERROR);
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

void SetBreakFlag(void) {
Flags |= BREAK_FLAG;
}	

void ClearBreakFlag(void) {
Flags &= ~BREAK_FLAG;
}	

int GetBreakFlag(void) {
return((Flags & BREAK_FLAG) >> 4);
}

int IncludeFile(char *filename) {
struct stat includestat;
char *tempbuf;
FILE *handle;
size_t newptr;
char *oldtextptr;

if(stat(filename,&includestat) == -1) return(FILE_NOT_FOUND);

handle=fopen(filename,"rb");
if(!handle) return(FILE_NOT_FOUND);

/* replace include statement with file contents */

tempbuf=malloc(includestat.st_size);		/* allocate temporary buffer */
strcpy(tempbuf,CurrentBufferPosition);		/* save from current buffer to end */

/* find relative distance from start of buffer and apply it to new buffer pointer */

newptr=(CurrentBufferPosition-FileBuffer);	/* find distance from start of buffer */
if(realloc(FileBuffer,(FileBufferSize+includestat.st_size)) == NULL) return(NO_MEM);	/* resize file buffer */

CurrentBufferPosition=(FileBuffer+newptr);	/* new file buffer position */

/* find start of include statement so it can be overwritten */

CurrentBufferPosition -= 2;		/* skip newline */

while(CurrentBufferPosition != FileBuffer) {
	if(*CurrentBufferPosition-- == '\n') break;
}

if(fread(CurrentBufferPosition,includestat.st_size,1,handle) != 1) return(READ_ERROR);		/* read include file to buffer */


/* copy text after included file */

oldtextptr=(CurrentBufferPosition+includestat.st_size)-1;
strcpy(oldtextptr,tempbuf);

oldtextptr += includestat.st_size;		/* point to end */

//CurrentBufferPosition=(oldtextptr-2);
free(tempbuf);

fclose(handle);
return(0);
}

int IsValidString(char *str) {
char *s;

if(*str != '"') return(FALSE);

s=(str+strlen(str))-1;		/* point to end */

if(*s != '"') return(FALSE);

return(TRUE);
}

int StripQuotesFromString(char *str,char *buf) {
char *s;
char *b;

if(IsValidString(str) == FALSE) return(-1);		/* not valid string */

/* copy filename without quotes */

s=str;
s++;

b=buf;

while(*s != '"') *b++=*s++;	/* copy character */

return(0);
}

