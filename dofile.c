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

/* File and statement processing functions  */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <setjmp.h>
#include "errors.h"
#include "size.h"
#include "evaluate.h"
#include "modeflags.h"
#include "variablesandfunctions.h"
#include "dofile.h"

int saveexprtrue=0;

functionreturnvalue retval;

char *currentptr=NULL;		/* current pointer in buffer */
char *endptr=NULL;		/* end of buffer */
char *readbuf=NULL;		/* buffer */
int bufsize=0;			/* size of buffer */
int ic=0;			/* number of included files */
char *TokenCharacters="+-*/<>=!%~|& \t()[],{};.";
int Flags=0;
char *CurrentFile[MAX_SIZE];
int returnvalue=0;

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
	if(!handle) {
	PrintError(FILE_NOT_FOUND);
	return(FILE_NOT_FOUND);
	}

	fseek(handle,0,SEEK_END);				/* get file size */
	filesize=ftell(handle);

	fseek(handle,0,SEEK_SET);			/* seek back to start */
	
	if(readbuf == NULL) {				/* first time */
	 readbuf=malloc(filesize+1);			/* allocate buffer */

	 if(readbuf == NULL) {			/* no memory */
	  PrintError(NO_MEM);
	  return(NO_MEM);
	 }
	}
	else
	{
	 if(realloc(readbuf,bufsize+filesize) == NULL) {		/* resize buffer */
	  PrintError(NO_MEM);
	  return(NO_MEM);
	 }
	}

	currentptr=readbuf;

	if(fread(readbuf,filesize,1,handle) != 1) {		/* read to buffer */
	 PrintError(READ_ERROR);
	 return(READ_ERROR);
	}
		
	endptr += filesize;		/* point to end */
	bufsize += filesize;

	strcpy(CurrentFile,filename);

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
char *savecurrentptr;
int lc=1;

SetCurrentFunctionLine(0);
	
if(LoadFile(filename) == -1) {
	PrintError(FILE_NOT_FOUND);
	return(FILE_NOT_FOUND);
}

SetIsRunningFlag();
SetIsFileLoadedFlag();

savecurrentptr=currentptr;		/* save current pointer */
currentptr=readbuf;

SetFunctionCallPtr(currentptr);

SetCurrentFunctionLine(1);

do {
	currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

	returnvalue=ExecuteLine(linebuf);
	if(returnvalue != 0) {
		ClearIsRunningFlag();
		return(returnvalue);
	}

	if(GetIsRunningFlag() == FALSE) {
		currentptr=savecurrentptr;
		return(NO_ERROR);	/* program ended */
	}

	memset(linebuf,0,MAX_SIZE);

	lc=GetCurrentFunctionLine();
	SetCurrentFunctionLine(++lc);

}    while(*currentptr != 0); 			/* until end */

currentptr=savecurrentptr;

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
char *eviltokens[MAX_SIZE][MAX_SIZE];
double exprone;
int statementcount;
int tc;
varsplit split;
varsplit assignsplit;
varval val;
char c;
int vartype;
int count;
int countx;
char *functionname[MAX_SIZE];
char *args[MAX_SIZE];
char *b;
char *d;
int start;
vars_t *varptr;
vars_t *assignvarptr;
int lc;

c=*lbuf;
if(c == '\r' || c == '\n' || c == 0) return(0);			/* blank line */

if(strlen(lbuf) > 1) {
	b=lbuf+strlen(lbuf)-1;
	if((*b == '\n') || (*b == '\r')) *b=0;
}

while(*lbuf == ' ' || *lbuf == '\t') lbuf++;	/* skip white space */

if(memcmp(lbuf,"//",2) == 0) return(0);		/* skip comments */

memset(tokens,0,MAX_SIZE*MAX_SIZE);

tc=TokenizeLine(lbuf,tokens,TokenCharacters);			/* tokenize line */
if(tc == -1) {
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

if(CheckSyntax(tokens,TokenCharacters,1,tc) == 0) {		/* check syntax */
	PrintError(SYNTAX_ERROR);
	return(0);
}

if(CallIfStatement(tc,tokens) == 0) return(0);			/* run statement if statement */

/* call user function */


if(CheckFunctionExists(tokens[0]) != -1) {	/* user function */
	return(CallFunction(tokens,0,tc));
} 

/*
 *
 * assignment
 *
 */

for(count=1;count<tc;count++) {

	if(strcmpi(tokens[count],"=") == 0) {

		if(CheckSyntax(tokens,TokenCharacters,1,tc) == FALSE) {		/* check syntax */
	  	  	PrintError(SYNTAX_ERROR);
	  		return(0);
	  	}

	 	ParseVariableName(tokens,0,count-1,&split);			/* split variable */  	

	 	SubstituteVariables(count+1,tc,tokens,tokens);

	 	if(strlen(split.fieldname) == 0) {			/* use variable name */
	 		vartype=GetVariableType(split.name);
		}
		else
		{
	 		vartype=GetFieldTypeFromUserDefinedType(split.name,split.fieldname);

			if(vartype == -1) {
		 		PrintError(TYPE_FIELD_DOES_NOT_EXIST);
		  		return(-1);
			}
		}
	
		c=*tokens[count+1];

		 if( ((c == '"') || (vartype == VAR_STRING))) {			/* string */  
		 	if(vartype == -1) {	
				CreateVariable(split.name,"STRING",split.x,split.y);		/* new variable */ 
		  	}
	 
		  	ConatecateStrings(count+1,tc,tokens,&val);					/* join all the strings on the line */

		  	UpdateVariable(split.name,split.fieldname,&val,split.x,split.y);		/* set variable */

		  	return(0);
		}

		/* number otherwise */

		 if(vartype == VAR_STRING) {		/* not string */
		 	PrintError(TYPE_ERROR);
		 	return(TYPE_ERROR);
		 }
	
		 exprone=EvaluateExpression(tokens,count+1,tc);

		 if(vartype == VAR_NUMBER) {
		 	val.d=exprone;
		 }
		 else if(vartype == VAR_STRING) {
			SubstituteVariables(count+1,count+1,tokens,tokens);

		 	if(tc == -1) return(-1);

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

			if((varptr == NULL) || (assignvarptr == NULL)) {
				PrintError(VARIABLE_DOES_NOT_EXIST);
				return(-1);
			}
		   
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

PrintError(INVALID_STATEMENT);
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
double exprone;
char c;
varval val;
int count;
int countx;
char *s[MAX_SIZE];
char *sptr;
int PrintFunctionFound;
char *udttokens[2][MAX_SIZE];

/* if string literal, string variable or function returning string */

if(((char) *tokens[1] == '"') || (GetVariableType(tokens[1]) == VAR_STRING) || (CheckFunctionExists(tokens[1]) == VAR_STRING) ) {
	count += ConatecateStrings(1,tc,tokens,&val);					/* join all the strings on the line */

	printf("%s",val.s);

	if(strcmp(tokens[tc-1],";") != 0) printf("\n");

	return(0);
}

SubstituteVariables(1,tc,tokens,tokens);

retval.val.type=0;
retval.val.d=EvaluateExpression(tokens,1,tc);

/* if it's a condition print True or False */

for(countx=1;countx<tc;countx++) {
	 if((strcmp(tokens[countx],">") == 0) || (strcmp(tokens[countx],"<") == 0) || (strcmp(tokens[countx],"=") == 0)) {

	     retval.val.type=0;
	     retval.val.d=EvaluateCondition(tokens,1,tc);

	     retval.val.d == 1 ? printf("True") : printf("False");
	     break;
	 } 
}
	 
if(countx == tc) printf("%.6g ",retval.val.d);	/* Not conditional */

if(strcmp(tokens[tc-1],";") != 0) printf("\n");
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
AddModule(tokens[1]);		/* load module */
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
SAVEINFORMATION *info;

if(tc < 1) {						/* not enough parameters */
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

SetFunctionFlags(IF_STATEMENT);

while(*currentptr != 0) {

	if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {  
		exprtrue=EvaluateCondition(tokens,1,tc);

		if(exprtrue == -1) return;

		if(exprtrue == 1) {
			saveexprtrue=exprtrue;

			do {
		   		currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
				if(*currentptr == 0) return(0);

				ExecuteLine(buf);

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) {
					PrintError(SYNTAX_ERROR);
					return(SYNTAX_ERROR);
				}

				if(strcmpi(tokens[0],"ENDIF") == 0) {
					SetFunctionFlags(IF_STATEMENT);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
	 	}
	 	else
	 	{
			do {
	   			currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
				if(*currentptr == 0) return(0);
	

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
				if(tc == -1) {
		 			PrintError(SYNTAX_ERROR);
		 			return(SYNTAX_ERROR);
				}

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
	   			currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
				if(*currentptr == 0) return(0);

				ExecuteLine(buf);

				tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

				if(tc == -1) {
		 			PrintError(SYNTAX_ERROR);
					return(SYNTAX_ERROR);	
				}

				if(strcmpi(tokens[0],"ENDIF") == 0) {
					ClearFunctionFlags(IF_STATEMENT);
					return(0);
				}

	  		} while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
		}
	}

	currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

	if(tc == -1) {
		PrintError(SYNTAX_ERROR);
		return(SYNTAX_ERROR);
	}

}

PrintError(ENDIF_NOIF);
return(ENDIF_NOIF);
}

/*
 * Endif statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((GetFunctionFlags() & IF_STATEMENT) == 0) PrintError(ENDIF_NOIF);
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
SAVEINFORMATION *info;

if(tc < 4) {						/* Not enough parameters */
	PrintError(NO_PARAMS);
	return(NO_PARAMS);
}


SetFunctionFlags(FOR_STATEMENT);

/* find end of variable name */

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"=") == 0) break;
}

if(count == tc) {		/* no = */
	ClearFunctionFlags(FOR_STATEMENT);

	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

ParseVariableName(tokens,1,count-1,&split);


//  0  1     2 3 4  5
// for count = 1 to 10

for(count=1;count<tc;count++) {
	if(strcmpi(tokens[count],"TO") == 0) break;		/* found to */
}

if(count == tc) {
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

for(countx=1;countx<tc;countx++) {
	if(strcmpi(tokens[countx],"step") == 0) break;		/* found step */
}

if(countx == tc) {
	steppos=1;
}
else
{
	SubstituteVariables(countx+1,tc,tokens,tokens);

	steppos=EvaluateExpression(tokens,countx+1,tc);
}

//  0   1    2 3 4  5
// for count = 1 to 10

SubstituteVariables(1,tc,tokens,tokens);

exprone=EvaluateExpression(tokens,3,count);			/* start value */
exprtwo=EvaluateExpression(tokens,count+1,countx);			/* end value */

if(GetVariableValue(split.name,split.fieldname,split.x,split.y,&loopx,split.fieldx,split.fieldy) == -1) {
	CreateVariable(split.name,"DOUBLE",split.x,split.y);
}

vartype=GetVariableType(split.name);			/* check if string */

switch(vartype) {
	case -1:
		loopcount.d=exprone;
		vartype=VAR_NUMBER;
		break;

	case VAR_STRING: 
		PrintError(TYPE_ERROR);
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

PushSaveInformation();					/* save line information */
currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */	

do {	   
	      returnvalue=ExecuteLine(buf);

		//      if(returnvalue != 0) {
	//  	 ClearIsRunningFlag();
	//		  return(returnvalue);
	//      }

		 //     if(GetIsRunningFlag() == FALSE) return(NO_ERROR);	/* program ended */
	    
	     d=*buf+(strlen(buf)-1);
	     if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
	     if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

	    tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	    if(tc == -1) {
		 PrintError(SYNTAX_ERROR);
		 return(SYNTAX_ERROR);
	    }

	    if(strcmpi(tokens[0],"NEXT") == 0) {	      	
	      	SetCurrentFunctionLine(GetSaveInformationLineCount());

		currentptr=GetSaveInformationBufferPointer();			/* get pointer to start of for statement */

	      /* increment or decrement counter */
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 1)) loopcount.d -= steppos;
	      	if( (vartype == VAR_NUMBER) && (ifexpr == 0)) loopcount.d += steppos;      
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 1)) loopcount.i -= steppos;
	      	if( (vartype == VAR_INTEGER) && (ifexpr == 0)) loopcount.i += steppos;      
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 1)) loopcount.f -=steppos;
	      	if( (vartype == VAR_SINGLE) && (ifexpr == 0)) loopcount.f += steppos;      

	      	UpdateVariable(split.name,split.fieldname,&loopcount,split.x,split.y);			/* set loop variable to next */	

	      	if(*currentptr == 0) {
	      		PopSaveInformation();				 
			ClearFunctionFlags(FOR_STATEMENT);

			PrintError(SYNTAX_ERROR);
			return(SYNTAX_ERROR);
	     	 }
	    	} 

		currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */	
	    } while(
		   ( (vartype == VAR_NUMBER) && (ifexpr == 1) && (loopcount.d > exprtwo)) ||
		   ((vartype == VAR_NUMBER) && (ifexpr == 0) && (loopcount.d < exprtwo)) ||
		   ((vartype == VAR_INTEGER) && (ifexpr == 1) && (loopcount.i > exprtwo)) ||
		   ((vartype == VAR_INTEGER) && (ifexpr == 0) && (loopcount.i < exprtwo)) ||
		   ((vartype == VAR_SINGLE) && (ifexpr == 1) && (loopcount.f > exprtwo)) ||
	    	   ((vartype == VAR_SINGLE) && (ifexpr == 0) && (loopcount.f < exprtwo))
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

SetFunctionFlags(FUNCTION_STATEMENT);

/* check return type */

for(count=1;count<tc;count++) {
	if((*tokens[count] >= '0' && *tokens[count] <= '9') && (GetFunctionReturnType() == VAR_STRING)) {
		PrintError(TYPE_ERROR);
		return(TYPE_ERROR);
 	}

 	if((*tokens[count] == '"') && (GetFunctionReturnType() != VAR_STRING)) {
  		PrintError(TYPE_ERROR);
  		return(TYPE_ERROR);
 	}

 	vartype=GetVariableType(tokens[count]);

 	if(vartype != -1) {
		if((GetFunctionReturnType() == VAR_STRING) && (vartype != VAR_STRING)) {
			PrintError(TYPE_ERROR);
   			return(TYPE_ERROR);
  		}

  		if((GetFunctionReturnType() != VAR_STRING) && (vartype == VAR_STRING)) {
   			PrintError(TYPE_ERROR);
   			return(TYPE_ERROR);
  		}

	}
}


retval.val.type=GetFunctionReturnType();		/* get return type */

if(GetFunctionReturnType() == VAR_STRING) {		/* returning string */
	ConatecateStrings(1,tc,tokens,&retval.val);		/* get strings */
	return(0);
}
else if(GetFunctionReturnType() == VAR_INTEGER) {		/* returning integer */
	SubstituteVariables(1,tc,tokens,tokens);

	retval.val.i=EvaluateExpression(tokens,1,tc);
}
else if(GetFunctionReturnType() == VAR_NUMBER) {		/* returning double */	 
	 SubstituteVariables(1,tc,tokens,tokens);

	 retval.val.d=EvaluateExpression(tokens,1,tc);
}
else if(GetFunctionReturnType() == VAR_SINGLE) {		/* returning single */
	SubstituteVariables(1,tc,tokens,tokens);

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
	if((GetFunctionFlags() & WHILE_STATEMENT) == 0) PrintError(WEND_NOWHILE);
	return(0);
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
	if((GetFunctionFlags() & FOR_STATEMENT) == 0) {
	 PrintError(NEXT_WITHOUT_FOR);
	 return(0);
	}

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
SAVEINFORMATION *info;

if(tc < 1) {						/* Not enough parameters */
	PrintError(SYNTAX_ERROR);
	return(SYNTAX_ERROR);
}

PushSaveInformation();					/* save line information */

memcpy(condition_tokens,tokens,((tc*MAX_SIZE)*MAX_SIZE)/sizeof(tokens));		/* save copy of condition */

condition_tc=tc;

SetFunctionFlags(WHILE_STATEMENT);
	
do {
	     currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

	     exprtrue=EvaluateCondition(condition_tokens,1,condition_tc);			/* do condition */

	     if(exprtrue == -1) {
	     	PrintError(BAD_CONDITION);
	     	return(BAD_CONDITION);
	     }

	     if(exprtrue == 0) {
	     		while(*currentptr != 0) {

	     		  	currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
			tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

			if(tc == -1) {
				PrintError(SYNTAX_ERROR);
		 		return(SYNTAX_ERROR);
			}

			if(strcmpi(tokens[0],"WEND") == 0) {
				PopSaveInformation();				 

				currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
				return(0);
			}
		}

	     }

	     tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
		if(tc == -1) {
		PrintError(SYNTAX_ERROR);
		return(SYNTAX_ERROR);
	    }

	     if(strcmpi(tokens[0],"WEND") == 0) {
		SetCurrentFunctionLine(GetSaveInformationLineCount());
	     }

	     returnvalue=ExecuteLine(buf);

	     if(returnvalue != 0) {
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
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) {
	PrintError(ELSE_WITHOUT_IF);
	return(ELSE_WITHOUT_IF);
}

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
if((GetFunctionFlags() & IF_STATEMENT) != IF_STATEMENT) {
	PrintError(ELSE_WITHOUT_IF);
	return(ELSE_WITHOUT_IF);
}

return(0);
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
	PrintError(ENDFUNCTION_NO_FUNCTION);
	return(ENDFUNCTION_NO_FUNCTION);
}

ClearFunctionFlags(FUNCTION_STATEMENT);
return(0);
}

/*
 * Include statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int include_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(LoadFile(tokens[1]) == -1) {
	PrintError(FILE_NOT_FOUND);
	return(FILE_NOT_FOUND);
}
	
}

/*
 * Break statement
 *
 * In: tc Token count
 * tokens Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];

if((strcmpi(tokens[1],"FOR") == 0) && (GetFunctionFlags() & FOR_STATEMENT)){
	PrintError(EXIT_FOR_WITHOUT_FOR);
	return(EXIT_FOR_WITHOUT_FOR);
}

if((strcmpi(tokens[1],"WHILE") == 0) && (GetFunctionFlags() & WHILE_STATEMENT)){
	PrintError(EXIT_WHILE_WITHOUT_WHILE);
	return(EXIT_WHILE_WITHOUT_WHILE);
}

/* find end of loop */
while(*currentptr != 0) {
	currentptr=ReadLineFromBuffer(currentptr,buf,MAX_SIZE);			/* get data */
	  
	if(*currentptr == 0) {			/* at end without next or wend */   

		if((strcmpi(tokens[1],"FOR") == 0) && (GetFunctionFlags() & FOR_STATEMENT)){
			PrintError(FOR_WITHOUT_NEXT);
			return(FOR_WITHOUT_NEXT);
		}
	
		if((strcmpi(tokens[1],"WHILE") == 0) && (GetFunctionFlags() & WHILE_STATEMENT)){
			PrintError(WHILE_WITHOUT_WEND);
			return(WHILE_WITHOUT_WEND);
		}
	}

	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) {
		PrintError(SYNTAX_ERROR);
		return(SYNTAX_ERROR);
	  	}

	if((strcmpi(tokens[1],"NEXT") == 0) && (GetFunctionFlags() & FOR_STATEMENT)){
	 	ClearFunctionFlags(FOR_STATEMENT);
		return(0);
	}

	if((strcmpi(tokens[1],"WEND") == 0) && (GetFunctionFlags() & WHILE_STATEMENT)){
	 	ClearFunctionFlags(WHILE_STATEMENT);
		return(0);
	}

   } 
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

if(retval.val.i != NO_ERROR) {
	 PrintError(retval.val.i);
}

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
SAVEINFORMATION *info;

if(((GetFunctionFlags() & FOR_STATEMENT)) || ((GetFunctionFlags() & WHILE_STATEMENT))) {
	currentptr=GetSaveInformationBufferPointer();
	return(0);
}

PrintError(CONTINUE_NO_LOOP);
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

if((GetUDT(tokens[1]) != NULL) || (IsValidVariableType(tokens[1]) != -1)) {		/* If type exists */
	PrintError(TYPE_EXISTS);
	return(TYPE_EXISTS); 
}

/* create user-defined type entry */

strcpy(addudt.name,tokens[1]);

addudt.field=malloc(sizeof(UserDefinedTypeField));	/* add first field */
if(addudt.field == NULL) {
	PrintError(NO_MEM);
	return(NO_MEM); 
}
	
fieldptr=addudt.field;

/* add user-defined type fields */
	
do {

	currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

	typetc=TokenizeLine(buf,typetokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) {
	 PrintError(SYNTAX_ERROR);
	 return(SYNTAX_ERROR); 
	}

	if(strcmpi(typetokens[0],"ENDTYPE") == 0) {		/* end of statement */
	AddUserDefinedType(&addudt);			/* add user-defined type */
		return(0);
	}

	if(strcmpi(typetokens[1],"AS") != 0) {			/* missing as */
	 PrintError(SYNTAX_ERROR);
	 return(SYNTAX_ERROR); 
	}

/* add field to user defined type */
	
	ParseVariableName(typetokens,0,typetc,&split);		/* split variable name */

	strcpy(fieldptr->fieldname,split.name);		/* copy name */
	
	fieldptr->xsize=split.x;				/* get x size of field variable */
	fieldptr->ysize=split.y;				/* get y size of field variable */
	fieldptr->type=IsValidVariableType(typetokens[2]);

/* get type of field variable */

	if(fieldptr->type == -1) {		/* is valid type */
		PrintError(INVALID_ARRAY_SUBSCRIPT);
		return(INVALID_ARRAY_SUBSCRIPT); 
	 }

	if(*currentptr == 0) break;		/* at end */
	
/* add link to next field */
	fieldptr->next=malloc(sizeof(UserDefinedTypeField));
	if(fieldptr->next == NULL) {
		PrintError(NO_MEM);
		return(NO_MEM); 
	}

	fieldptr=fieldptr->next;

} while(*currentptr != 0);

return(-1);
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
	PrintError(SYNTAX_ERROR);
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
	    *d=*token++;

	    if(*d == '"') break;		/* quoted text */	

	    d++;
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
	char *t=token;
	int statementcount;

	if(*token == 0) return(TRUE);
	
	s=sep;

	 while(*s != 0) {
	  if(*s++ == *token) return(TRUE);
	 }

	statementcount=0;

	if(IsStatement(token) == TRUE) return(TRUE);

	return(FALSE);
}

/*
 * Check syntax
 *
 * In: tokens		Tokens to check
		separators	Separator characters to check against
		start		Start in array
		end		End in array
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

/* check if brackets are balanced */

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"(") == 0) bracketcount++;
	if(strcmp(tokens[count],")") == 0) bracketcount--;

	if(strcmp(tokens[count],"[") == 0) squarebracketcount++;
	if(strcmp(tokens[count],"]") == 0) squarebracketcount--;

	if(strcmp(tokens[count],",") == 0) {
		if(strcmpi(tokens[0],"PRINT") == 0) return(TRUE);
		if(bracketcount == 0) return(FALSE);
	}
}

if((bracketcount != 0) || (squarebracketcount != 0)) return(FALSE);

for(count=start;count<end;count += 2) {

/* check if two separators are together */

	if(*tokens[count] == 0) break;

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

	  /* check if two non-separator tokens are next to each other */

for(count=start;count<end;count++) {   
	if((IsSeperator(tokens[count],separators) == 0) && (count < end) && (IsSeperator(tokens[count+1],separators) == 0)) return(FALSE);
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
return(currentptr);
}

char *SetCurrentBufferAddress(char *addr) {
currentptr=addr;
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
Flags |= ~INTERACTIVE_MODE_FLAG;
}

void GetCurrentFile(char *buf) {
strcpy(buf,CurrentFile);
}

void SetCurrentFile(char *buf) {
strcpy(CurrentFile,buf);
}

void SetCurrentBufferPosition(char *pos) {
currentptr=pos;
}

char *GetCurrentBufferPosition(void) {
return(currentptr);
}

void GetTokenCharacters(char *tbuf) {
strcpy(tbuf,TokenCharacters);
}


