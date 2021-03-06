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

/*
 * File and statement processing functions 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "define.h"

char *llcerrs[] = { "No error","File not found","No parameters for statement","Bad expression",\
		    "IF statement without ELSEIF or ENDIF","FOR statement without NEXT",\
		    "WHILE without WEND","ELSE without IF","ENDIF without IF","ENDFUNCTION without FUNCTION",\
		    "Invalid variable name","Out of memory","BREAK outside FOR or WHILE loop","Read error","Syntax error",\
		    "Error calling library function","Invalid statement","Nested function","ENDFUNCTION without FUNCTION",\
		    "NEXT without FOR","WEND without WHILE","Duplicate function","Too few arguments",\
		    "Invalid array subscript","Type mismatch","Invalid type","CONTINUE without FOR or WHILE","ELSEIF without IF",\
		    "Invalid condition","Invalid type in declaration","Missing XSCRIPT_MODULE_PATH" };

char *ReadLineFromBuffer(char *buf,char *linebuf,int size);

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int for_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int include_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int break_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int type_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);

int saveexprtrue=0;
extern int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE]);
varval retval;

/* statements */
			  
statement statements[] = { { "IF",&if_statement },\
      { "ELSE",&else_statement },\
      { "ENDIF",&endif_statement },\
      { "FOR",&for_statement },\
      { "WHILE",&while_statement },\
      { "WEND",&wend_statement },\
      { "PRINT",&print_statement },\
      { "IMPORT",&import_statement },\ 
      { "END",&end_statement },\ 
      { "FUNCTION",&function_statement },\ 
      { "ENDFUNCTION",&endfunction_statement },\ 
      { "RETURN",&return_statement },\ 
      { "INCLUDE",&include_statement },\ 
      { "DECLARE",&declare_statement },\
      { "RUN",&run_statement },\
      { "CONTINUE",&continue_statement },\
      { "NEXT",&next_statement },\
      { "BREAK",&break_statement },\
      { "AS",&bad_keyword_as_statement },\
      { "TO",&bad_keyword_as_statement },\
      { "STEP",&bad_keyword_as_statement },\
      { "THEN",&bad_keyword_as_statement },\
      { "AS",&bad_keyword_as_statement },\
      { "DOUBLE",&bad_keyword_as_statement },\
      { "STRING",&bad_keyword_as_statement },\
      { "INTEGER",&bad_keyword_as_statement },\
      { "SINGLE",&bad_keyword_as_statement },\
      { NULL,NULL } };

extern functions *currentfunction;
extern functions *funcs;
extern char *vartypenames[];

char *currentptr=NULL;		/* current pointer in buffer */
char *endptr=NULL;		/* end of buffer */
char *readbuf=NULL;		/* buffer */
int bufsize=0;			/* size of buffer */
int ic=0;			/* number of included files */

include includefiles[MAX_INCLUDE];	/* included files */

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
 if(!handle) return(-1);						/* can't open */

 fseek(handle,0,SEEK_END);				/* get file size */
 filesize=ftell(handle);

 fseek(handle,0,SEEK_SET);			/* seek back to start */
 
 if(readbuf == NULL) {
  readbuf=malloc(filesize+1);		/* allocate buffer */

  if(readbuf == NULL) {
   PrintError(NO_MEM);
   return(NO_MEM);
  }
 }
 else
 {
  if(realloc(readbuf,bufsize+filesize) == NULL) {
   PrintError(NO_MEM);
   return(NO_MEM);
  }
 }

 currentfunction->saveinformation[0].bufptr=readbuf;
 currentfunction->saveinformation[0].lc=0;
 currentfunction->nestcount=0;

 currentptr=readbuf;

 if(fread(readbuf,filesize,1,handle) != 1) {		/* read to buffer */
  PrintError(READ_ERROR);
  return(READ_ERROR);
 }


 strcpy(includefiles[ic].filename,filename);
		
 endptr += filesize;		/* point to end */
 bufsize += filesize;

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

 //printf("debug\n");

 includefiles[ic].lc=0;
 
 if(LoadFile(filename) == -1) {
  PrintError(FILE_NOT_FOUND);
  return(FILE_NOT_FOUND);
}

/* loop through lines and execute */

do {
 currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;

 currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

 currentfunction->saveinformation[currentfunction->nestcount].lc=includefiles[ic].lc;

 if(*currentptr == 0) return(NO_ERROR);

 ExecuteLine(linebuf);

 memset(linebuf,0,MAX_SIZE);

 includefiles[ic].lc++;
}    while(*currentptr != 0); 			/* until end */

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
 varval val;
 char c;
 int vartype;
 int count;
 int countx;
 char *functionname[MAX_SIZE];
 char *args[MAX_SIZE];
 char *b;
 char *d;

 includefiles[ic].lc++;						/* increment line counter */

 /* return if blank line */

c=*lbuf;

if(c == '\r' || c == '\n' || c == 0) return;			/* blank line */

if(strlen(lbuf) > 1) {
 b=lbuf+strlen(lbuf)-1;
 if((*b == '\n') || (*b == '\r')) *b=0;
}

while(*lbuf == ' ' || *lbuf == '\t') lbuf++;	/* skip white space */

if(memcmp(lbuf,"//",2) == 0) return;		/* skip comments */

memset(tokens,0,MAX_SIZE*MAX_SIZE);

tc=TokenizeLine(lbuf,tokens," \009");			/* tokenize line */

/* do statement */

statementcount=0;

do {
 if(statements[statementcount].statement == NULL) break;

// printf("%s %s\n",statements[statementcount].statement,tokens[0]);

 if(strcmpi(statements[statementcount].statement,tokens[0]) == 0) {  
  if(statements[statementcount].call_statement(tc,tokens) == -1) exit(-1);		/* call statement and exit if error */
  statementcount=0;

  return(0);
 }
 
 statementcount++;

} while(statements[statementcount].statement != NULL);
	
/* call user function */

b=tokens[0];		/* get function name */
d=functionname;

while(*b != 0) {
 if(*b == '(') {
  d=args;		/* copy to args */
  b++;
 }

 *d++=*b++;
}

d--;
if(*d == ')') *d=' ';	// no )


if(CheckFunctionExists(functionname) != -1) {	/* user function */
 if(CallFunction(functionname,args) == -1) exit(-1);
} 


/*
 *
 * assignment
 *
 */
for(count=1;count<tc;count++) {

 if(strcmpi(tokens[count],"=") == 0) {

	 ParseVariableName(tokens[count-1],&split);			/* split variable */

	 vartype=GetVariableType(split.name);

	 c=*tokens[count+1];

	 if((c == '"') || GetVariableType(split.name) == VAR_STRING) {			/* string */  
	  if(vartype == -1) {
		CreateVariable(split.name,VAR_STRING,split.x,split.y);		/* new variable */ 
	  }
	  else
	  {  
	   PrintError(TYPE_ERROR);
	   return(TYPE_ERROR);
	  }

	  ConatecateStrings(count+1,tc,tokens,&val);					/* join all the strings on the line */

	  UpdateVariable(split.name,&val,split.x,split.y);		/* set variable */

	  return;
	 }

	/* number otherwise */

	 if(vartype == VAR_STRING) {		/* not string */
	  PrintError(TYPE_ERROR);
	  return(TYPE_ERROR);
	 }

	 exprone=doexpr(tokens,count+1,tc);

	 if(vartype == VAR_NUMBER) {
	  val.d=exprone;
	 }
 	 else if(vartype == VAR_STRING) {
	  SubstituteVariables(count+1,count+1,tokens); 
	  strcpy(val.s,tokens[count+1]);  
	 }
	 else if(vartype == VAR_INTEGER) {
	  val.i=exprone;
	 }
	 else if(vartype == VAR_SINGLE) {
	  val.f=exprone;
	 }
	 else
	 {
	  val.d=exprone;
	 }


	 if(vartype == -1) {		/* new variable */ 
	  CreateVariable(split.name,VAR_NUMBER,split.x,split.y);			/* create variable */
	  UpdateVariable(split.name,&val,split.x,split.y);
	  return;
	 }

	 UpdateVariable(split.name,&val,split.x,split.y);

	 return;
  } 

}

PrintError(INVALID_STATEMENT);

return;
}

/*
 * Declare function statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *valptr;
char *functionname[MAX_SIZE];
char *args[MAX_SIZE];
char *b;
char *d;
int count;
int vartype;
int countx;
char *buf[MAX_SIZE];

b=tokens[1];		/* get function name */
d=functionname;

while(*b != 0) {	/* copy until ) */
 if(*b == '(') {
  b++;
  break;
 }

 *d++=*b++;
}

*d=0;

/* copy remainder in tokens[1] */
d=args;

while(*b != 0) {
//	printf("%c\n",*b);

	 if(*b == ')') {		/* copy until ) */
		*d=0;
		break;		/* at end of args */
	 }

 *d++=*b++;
}

count++;

while(count < tc) {
 if(strcmpi(tokens[count],"AS") == 0) {		/* array as type */
  vartype=0;

  while(vartypenames[vartype] != NULL) {
    if(strcmpi(vartypenames[vartype],tokens[3]) == 0) break;	/* found type */
   
   vartype++;
  }

  if(vartypenames[vartype] == NULL) {		/* invalid type */
   PrintError(BAD_TYPE);
   return(BAD_TYPE);
  }

  break;
 }
 else
 {
  vartype=VAR_NUMBER;
  break;
 }

 count++;
}

if((strlen(tokens[2]) > 0) && (strcmpi(tokens[2],"AS") != 0)) {			/* not as */
  PrintError(SYNTAX_ERROR);
  return;
}

if(strcmpi(tokens[2],"AS") == 0) {			/* return type */
 vartype=CheckVariableType(tokens[3]);		/* get variable type */  

 if(vartype == -1) {				/* invalid variable type */
  PrintError(BAD_TYPE);
  return(-1);
 }
}
else
{
 vartype=VAR_NUMBER;
}

DeclareFunction(functionname,args,vartype);

return;
} 

/*
 * Print statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char c;
varval val;
int countx;
varsplit split;
double exprone;

SubstituteVariables(1,tc,tokens);

ParseVariableName(tokens[1],&split);

c=*tokens[1];

/* if string literal, string variable or function returning string */

if((c == '"') || (GetVariableType(tokens[1]) == VAR_STRING) || (CheckFunctionExists(split.name) == VAR_STRING) ) {	/* user function */ 
 ConatecateStrings(1,tc,tokens,&val);					/* join all the strings on the line */
 
 printf("%s\n",val.s);
 return;
}

for(countx=1;countx<tc;countx++) {
 printf("%s\n",tokens[countx]);
}

exprone=doexpr(tokens,1,tc);
printf("z=%.6g\n",exprone);

return;
}

/*
 * Import statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int retval=AddModule(tokens[1]);

if(retval > 0) {		/* load module */
 PrintError(retval);
}

return;
}

/*
 * If statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int count;
int countx;
char *d;
int exprtrue;

if(tc < 1) {						/* not enough parameters */
 PrintError(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

currentfunction->stat |= IF_STATEMENT;

while(*currentptr != 0) {

if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {
  exprtrue=EvaluateCondition(tokens,1,tc);
  if(exprtrue == -1) {
   PrintError(BAD_CONDITION);
   return(-1);
  }

if(exprtrue == 1) {
		saveexprtrue=exprtrue;

		do {
    		currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

		if(*currentptr == 0) return;

		ExecuteLine(buf);

		TokenizeLine(buf,tokens," \009");			/* tokenize line */

		if(strcmpi(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF") != 0)  && (strcmpi(tokens[0],"ELSE") != 0));
  }
}

 if((strcmpi(tokens[0],"ELSE") == 0)) {

  if(saveexprtrue == 0) {
	    do {
    		currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
		if(*currentptr == 0) return;

		ExecuteLine(buf);

		TokenizeLine(buf,tokens," \009");			/* tokenize line */

		if(strcmpi(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
 }
}

 currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
 TokenizeLine(buf,tokens," \009");			/* tokenize line */

}

//PrintError(ENDIF_NOIF);
}

/*
 * Endif statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & IF_STATEMENT) == 0) PrintError(ENDIF_NOIF);
}

/*
 * For statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
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

if(tc < 4) {						/* Not enough parameters */
 PrintError(NO_PARAMS);
 return(NO_PARAMS);
}

currentfunction->stat |= FOR_STATEMENT;

if(strcmpi(tokens[2],"=") != 0) {
 PrintError(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

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
 steppos=doexpr(tokens,countx+1,tc);
}

ParseVariableName(tokens[1],&split);

//  0   1    2 3 4  5
// for count = 1 to 10

exprone=doexpr(tokens,3,count);			/* start value */
exprtwo=doexpr(tokens,count+1,countx);			/* end value */

if(GetVariableValue(split.name,&loopx) == -1) {		/* new variable */  
 CreateVariable(split.name,VAR_NUMBER,split.x,split.y);
}

vartype=GetVariableType(split.name);			/* check if string */

switch(vartype) {
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

UpdateVariable(split.name,&loopcount,split.x,split.y);			/* set loop variable to next */	

if(exprone >= exprtwo) {
 ifexpr=1;
}
else
{
 ifexpr=0;
}

currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;
currentfunction->saveinformation[currentfunction->nestcount].lc=includefiles[ic].lc;

// printf("startptr=%lX\n",currentptr);

 while((ifexpr == 1 && loopcount.d > exprtwo) || (ifexpr == 0 && loopcount.d < exprtwo)) {
	    currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */	

	    ExecuteLine(buf);

	     d=*buf+(strlen(buf)-1);
             if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
             if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

 	     TokenizeLine(buf,tokens," \009");			/* tokenize line */

  	     if(strcmpi(tokens[0],"NEXT") == 0) {

	      includefiles[ic].lc;currentfunction->saveinformation[currentfunction->nestcount].lc;

	      currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;		/* restore position */   	    

			
	      if(ifexpr == 1) loopcount.d=loopcount.d-steppos;					/* increment or decrement counter */
 	      if(ifexpr == 0) loopcount.d=loopcount.d+steppos;      
		
	      UpdateVariable(split.name,&loopcount,split.x,split.y);			/* set loop variable to next */	

              if(*currentptr == 0) {
	          if(currentfunction->nestcount-1 > 0) currentfunction->nestcount--;
               
	       currentfunction->stat &= FOR_STATEMENT;
	       PrintError(SYNTAX_ERROR);
	       return(SYNTAX_ERROR);
              }

		
	    }	   
     }

  if(currentfunction->nestcount-1 > 0) currentfunction->nestcount--;
  currentfunction->stat &= FOR_STATEMENT;
  return;
 }

/*
 * Return statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

extern int callpos;

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 int count;
 int vartype;

 currentfunction->stat &= FUNCTION_STATEMENT;

/* check return type */

 for(count=1;count<tc;count++) {
  if((*tokens[count] >= '0' && *tokens[count] <= '9') && (currentfunction->return_type == VAR_STRING)) {
   PrintError(TYPE_ERROR);
   return(-1);
  }

  if((*tokens[count] == '"') && (currentfunction->return_type != VAR_STRING)) {
   PrintError(TYPE_ERROR);
   return(-1);
  }

  vartype=GetVariableType(tokens[count]);

  if(vartype != -1) {
   if((currentfunction->return_type == VAR_STRING) && (vartype != VAR_STRING)) {
    PrintError(TYPE_ERROR);
    return(-1);
   }

   if((currentfunction->return_type != VAR_STRING) && (vartype == VAR_STRING)) {
    PrintError(TYPE_ERROR);
    return(-1);
   }

 }
}

 retval.type=currentfunction->return_type;		/* get return type */

 if(currentfunction->return_type == VAR_STRING) {		/* returning string */
  ConatecateStrings(1,tc,tokens,&retval);		/* get strings */
  return;
 }
 else if(currentfunction->return_type == VAR_INTEGER) {		/* returning integer */
	 retval.i=doexpr(tokens,1,tc);
 }
 else if(currentfunction->return_type == VAR_NUMBER) {		/* returning double */
	 retval.d=doexpr(tokens,1,tc);
 }
 else if(currentfunction->return_type == VAR_SINGLE) {		/* returning single */
	 retval.f=doexpr(tokens,1,tc);	
 }

ReturnFromFunction();			/* return */
 return;
}

int get_return_value(varval *val) {
 val=&retval;
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & WHILE_STATEMENT) == 0) PrintError(WEND_NOWHILE);
 return;
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & FOR_STATEMENT) == 0) {
  PrintError(NEXT_NO_FOR);
  return;
 }
}

/*
 * While statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int exprtrue;
char *d;
int count;
char *condition_tokens[MAX_SIZE][MAX_SIZE];
int condition_tc;

if(tc < 1) {						/* Not enough parameters */
 PrintError(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

//asm("int $3");

currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;
currentfunction->saveinformation[currentfunction->nestcount].lc=includefiles[ic].lc;

memcpy(condition_tokens,tokens,(tc*MAX_SIZE)*MAX_SIZE);		/* save copy of condition */
condition_tc=tc;

currentfunction->stat |= WHILE_STATEMENT;
 
do {
      currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

      exprtrue=EvaluateCondition(condition_tokens,1,condition_tc);			/* do condition */

      if(exprtrue == -1) {
       PrintError(BAD_CONDITION);
       return(-1);
      }

      if(exprtrue == 0) {
       while(*currentptr != 0) {

        currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
        tc=TokenizeLine(buf,tokens," \009");

        if(strcmpi(tokens[0],"WEND") == 0) {
         currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
	 return;
        }
       }

      }

      tc=TokenizeLine(buf,tokens," \009");

      if(strcmpi(tokens[0],"WEND") == 0) {
       includefiles[ic].lc;currentfunction->saveinformation[currentfunction->nestcount].lc;
       currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;
      }

     ExecuteLine(buf);
  } while(exprtrue == 1);
  

}

/*
 * End statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 return(atoi(tokens[1]));
}

/*
 * Else statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat& IF_STATEMENT) != IF_STATEMENT) {
  PrintError(ELSE_NOIF);
  return(ELSE_NOIF);
 }

return;
}

/*
 * Endfunction statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((currentfunction->stat & FUNCTION_STATEMENT) != FUNCTION_STATEMENT) {
 PrintError(ENDFUNCTION_NO_FUNCTION);
 return(ENDFUNCTION_NO_FUNCTION);
}

currentfunction->stat|= FUNCTION_STATEMENT;
return;
}

/*
 * Include statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
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
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int break_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 char *buf[MAX_SIZE];
 char *d;

 if(((currentfunction->stat & FOR_STATEMENT) == 0) && ((currentfunction->stat & WHILE_STATEMENT) == 0)) {
  PrintError(INVALID_BREAK);
  return(INVALID_BREAK);
 }

 if((currentfunction->stat & FOR_STATEMENT) || (currentfunction->stat & WHILE_STATEMENT)) {
  while(*currentptr != 0) {
   currentptr=ReadLineFromBuffer(currentptr,buf,MAX_SIZE);			/* get data */
   
   if(*currentptr == 0) {			/* at end without next or wend */   
    if((currentfunction->stat & FOR_STATEMENT) == 0) {
    PrintError(FOR_NO_NEXT);
    return(FOR_NO_NEXT);
   }

    if((currentfunction->stat & WHILE_STATEMENT) == 0) {
    PrintError(WEND_NOWHILE);
    return(WEND_NOWHILE);
   }
  }

   TokenizeLine(buf,tokens," \009");

   if((strcmpi(tokens[0],"WEND") == 0) || (strcmpi(tokens[0],"NEXT") == 0)) {
    if((strcmpi(tokens[0],"WEND") == 0)) currentfunction->stat &= WHILE_STATEMENT;
    if((strcmpi(tokens[0],"NEXT") == 0)) currentfunction->stat &= FOR_STATEMENT;
  
    currentptr=ReadLineFromBuffer(currentptr,buf,MAX_SIZE);			/* get data */
    return;
   }
  }
 } 
}

/*
 * Declare statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 varsplit split;
 int vartype;

 ParseVariableName(tokens[1],&split);

 if(strcmpi(tokens[2],"AS") == 0) {		/* array as type */
  vartype=CheckVariableType(tokens[3]);		/* get variable type */  
 
  if(vartype == -1) {				/* invalid variable type */
   PrintError(BAD_TYPE);
   return(-1);
  }
 }
 else
 {
  vartype=VAR_NUMBER;
 }

 CreateVariable(split.name,vartype,split.x,split.y);

}

/*
 * Run statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(ExecuteFile(tokens[1]) == -1) {
  PrintError(FILE_NOT_FOUND);
  return(FILE_NOT_FOUND);
 }

}

/*
 * Continue statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(((currentfunction->stat & FOR_STATEMENT)) || ((currentfunction->stat & WHILE_STATEMENT))) {
  currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;
  return;
 }

 PrintError(CONTINUE_NO_LOOP);
 return(CONTINUE_NO_LOOP);
}

/*
 * Non-statement keyword as statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 PrintError(SYNTAX_ERROR);
}

/*
 * Declare function statement
 *
 * In: char *linebuf			Line to tokenize
       char *tokens[MAX_SIZE][MAX_SIZE]	Token array output
 *
 * Returns -1 on error or token count on success
 *
 */

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split) {
char *token;
char *tokenx;
int tc;
int count;
char c;
char *d;
int x;
char *z;
char *s;
tc=0;

token=linebuf;

while(*token == ' ' || *token == '\t') token++;	/* skip leading whitespace characters */

/* tokenize line */

 tc=0;
 memset(tokens,0,10*MAX_SIZE);				/* clear line */
 
 d=tokens[0];

while(*token != 0) {
 if((*token == '"') || (*token == '(') || (*token == '[') ) {		/* quoted text */
   
   *d++=*token++;
   while(*token != 0) {
    *d=*token++;

    if((*d == '"') || (*d == ')') || (*d == ']') ) {		/* quoted text */
	
	break;
    }

    d++;
  }

 }

  s=split;

  while(*s != 0) {
 
   if((*token == *s)) {		/* token found */

    tc++;
    d=tokens[tc]; 		/* new token */
   }
   else
   {
    *d++=*token;
   }

   s++;
 }

 token++;
}

return(tc+1);
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

return;
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

memset(linebuf,0,size);

do {
 if(count++ == size) break;

  *lineptr++=*buf++;
  b=buf;
  b--;

//  if(*b == 0) break;
  if(*b == '\n' || *b == '\r') break;

} while(*b != 0);		/* until end of line */


b=linebuf;
b += strlen(linebuf);
b--;

*b=0;

return(buf);			/* return new position */
}

/*
 * Display error
 *
 * In: int llcerr			Error number
 *
 * Returns: Nothing
 *
 */

int PrintError(int llcerr) {
 if(*includefiles[ic].filename == 0) {			/* if in interactive mode */
  printf("%s %s\n",currentfunction->name,llcerrs[llcerr]);
 }
 else
 {
//  printf("line=%lX\n",currentptr);
//  asm("int $3");

  printf("%s %d: %s %s\n",includefiles[ic].filename,includefiles[ic].lc,currentfunction->name,llcerrs[llcerr]);
 }
}

/*
 * Compare string case insensitively
 *
 * In: char *source		First string
       char *dest		Second string
 *
 * Returns: Nothing
 *
 */
int strcmpi(char *source,char *dest) {
 char a,b;
 char *sourcetemp[MAX_SIZE];
 char *desttemp[MAX_SIZE];

 strcpy(sourcetemp,source);
 strcpy(desttemp,dest);

 touppercase(sourcetemp);
 touppercase(desttemp);

 return(strcmp(sourcetemp,desttemp));
}
