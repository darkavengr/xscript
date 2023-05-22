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
#include <stdbool.h>
#include <setjmp.h>
#include "define.h"
#include "dofile.h"

char *llcerrs[] = { "No error","File not found","Missing parameters in statement","Invalid expression",\
		    "IF statement without ENDIF","FOR statement without NEXT",\
		    "WHILE without WEND","ELSE without IF","ENDIF without IF","ENDFUNCTION without FUNCTION",\
		    "Invalid variable name","Out of memory","BREAK outside FOR or WHILE loop","Read error","Syntax error",\
		    "Error calling library function","Invalid statement","Nested function","ENDFUNCTION without FUNCTION",\
		    "NEXT without FOR","WEND without WHILE","Duplicate function name","Too few arguments to function",\
		    "Invalid array subscript","Type mismatch","Invalid variable type","CONTINUE without FOR or WHILE","ELSEIF without IF",\
		    "Invalid condition","Invalid type in declaration","Missing XSCRIPT_MODULE_PATH path","Variable already exists",
		    "Variable not found","EXIT FOR without FOR","EXIT WHILE without WHILE","FOR without NEXT" };

int saveexprtrue=0;
varval retval;

/* statements */
			  
statement statements[] = { { "IF","ENDIF",&if_statement,TRUE},\
      { "ELSE",NULL,&else_statement,FALSE},\
      { "ELSEIF",NULL,&elseif_statement,FALSE},\
      { "ENDIF",NULL,&endif_statement,FALSE},\
      { "FOR","NEXT",&for_statement,TRUE},\
      { "WHILE","WEND",&while_statement,TRUE},\
      { "WEND",NULL,&wend_statement,FALSE},\
      { "PRINT",NULL,&print_statement,FALSE},\
      { "IMPORT",NULL,&import_statement,FALSE},\ 
      { "END",NULL,&end_statement,FALSE},\ 
      { "FUNCTION","ENDFUNCTION",&function_statement,TRUE},\ 
      { "ENDFUNCTION",NULL,&endfunction_statement,FALSE},\ 
      { "RETURN",NULL,&return_statement,FALSE},\ 
      { "INCLUDE",NULL,&include_statement,FALSE},\ 
      { "DECLARE",NULL,&declare_statement,FALSE},\
      { "ITERATE",NULL,&iterate_statement,FALSE},\
      { "NEXT",NULL,&next_statement,FALSE},\
      { "EXIT",NULL,&exit_statement,FALSE},\
      { "AS",NULL,&bad_keyword_as_statement,FALSE},\
      { "TO",NULL,&bad_keyword_as_statement,FALSE},\
      { "STEP",NULL,&bad_keyword_as_statement,FALSE},\
      { "THEN",NULL,&bad_keyword_as_statement,FALSE},\
      { "AS",NULL,&bad_keyword_as_statement,FALSE},\
      { "DOUBLE",NULL,&bad_keyword_as_statement,FALSE},\
      { "STRING",NULL,&bad_keyword_as_statement,FALSE},\
      { "INTEGER",NULL,&bad_keyword_as_statement,FALSE},\
      { "SINGLE",NULL,&bad_keyword_as_statement,FALSE},\
      { "AND",NULL,&bad_keyword_as_statement,FALSE},\
      { "OR",NULL,&bad_keyword_as_statement,FALSE},\
      { "NOT",NULL,&bad_keyword_as_statement,FALSE},\
      { "QUIT",NULL,&quit_command,FALSE},\
      { "VARIABLES",NULL,&variables_command,FALSE},\
      { "CONTINUE",NULL,&continue_command,FALSE},\
      { "LOAD",NULL,&load_command,FALSE},\
      { "RUN",NULL,&run_command,FALSE},\
      { "SET",NULL,&set_command,FALSE},\
      { "CLEAR",NULL,&clear_command,FALSE},\
      { NULL,NULL,NULL } };

extern FUNCTIONCALLSTACK *currentfunction;
extern FUNCTIONCALLSTACK *funcs;
extern char *vartypenames[];
extern jmp_buf savestate;

char *currentptr=NULL;		/* current pointer in buffer */
char *endptr=NULL;		/* end of buffer */
char *readbuf=NULL;		/* buffer */
int bufsize=0;			/* size of buffer */
int ic=0;			/* number of included files */
char *TokenCharacters="+-*/<>=!%~|& \t()[],{}";
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
 if(!handle) {
	PrintError(FILE_NOT_FOUND);
	return(-1);
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

 SetIsRunningFlag(FALSE);
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
 SAVEINFORMATION *info;

 currentfunction->lc=0;
 
 if(LoadFile(filename) == -1) {
  PrintError(FILE_NOT_FOUND);
  return(FILE_NOT_FOUND);
}

/* loop through lines and execute */

do {
 currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

 ExecuteLine(linebuf);			/* run statement */

 memset(linebuf,0,MAX_SIZE);

 currentfunction->lc++;
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
 int start;

 currentfunction->lc++;						/* increment line counter */

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

tc=TokenizeLine(lbuf,tokens,TokenCharacters);			/* tokenize line */
if(tc == -1) {
 PrintError(SYNTAX_ERROR);
 return(-1);
}

if(CheckSyntax(tokens,TokenCharacters,1,tc) == 0) {		/* check syntax */
 PrintError(SYNTAX_ERROR);
 return;
}

/* check if statement is valid by searching through struct of statements*/

statementcount=0;

do {
 if(statements[statementcount].statement == NULL) break;

/* found statement */

 if(strcmpi(statements[statementcount].statement,tokens[0]) == 0) {  

  if(statements[statementcount].call_statement(tc,tokens) > 0) return(-1);		/* call statement and exit if error */
  statementcount=0;

  return(0);
 }
 
 statementcount++;

} while(statements[statementcount].statement != NULL);

/* call user function */


if(CheckFunctionExists(tokens[0]) != -1) {	/* user function */
 CallFunction(tokens,0,tc);
 return;
} 


/*
 *
 * assignment
 *
 */

for(count=1;count<tc;count++) {

  if(strcmpi(tokens[count],"=") == 0) {
	  if(CheckSyntax(tokens,1,tc) == FALSE) {		/* check syntax */
   	   PrintError(SYNTAX_ERROR);
	   return;
	  }

	 ParseVariableName(tokens,0,count-1,&split);			/* split variable */  	
	 vartype=GetVariableType(split.name);

	 c=*tokens[count+1];

	 if((c == '"') || (vartype == VAR_STRING)) {			/* string */  
	  if(vartype == -1) {	
		CreateVariable(split.name,VAR_STRING,split.x,split.y);		/* new variable */ 
	  }
	  else if(vartype != VAR_STRING) {
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
	  SubstituteVariables(count+1,count+1,tokens,tokens); 
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
return(-1);
}

/*
 * Declare function statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {

DeclareFunction(&tokens[1],tc-1);

return;
} 

/*
 * Print statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
double exprone;
char c;
varval val;
int start;
int end;
int count;
int countx;
char *s[MAX_SIZE];
char *sptr;
double retval=0;

start=1;
SubstituteVariables(1,tc,tokens);  

for(count=1;count < tc;count++) {
 c=*tokens[count];

 /* if string literal, string variable or function returning string */

 if((c == '"') || (GetVariableType(tokens[count]) == VAR_STRING) || (CheckFunctionExists(tokens[count]) == VAR_STRING) ) {
    count += ConatecateStrings(1,tc,tokens,&val);					/* join all the strings on the line */

    printf("%s ",val.s);
    return;

 }
 else
 {
  start=count;
  end=start+1;

  while(end < tc) {
   if(strcmp(tokens[end],"(") == 0) {
	  while(end < tc) {
		   if(strcmp(tokens[end++],")") == 0) break;
          }
   }

   if(strcmp(tokens[end],",") == 0) break;
   end++;
  }

  retval=doexpr(tokens,start,end);

/* if it's a condition print True or False */

  for(countx=start;countx<end;countx++) {
    if((strcmp(tokens[countx],">") == 0) || (strcmp(tokens[countx],"<") == 0) || (strcmp(tokens[countx],"=") == 0)) {
      retval == 1 ? printf("True") : printf("False");
      break;
    } 
  }
  
  if(countx == end) printf("%.6g ",retval);	/* Not conditional */

  count=end;
 }
}

if(strcmp(tokens[end],";") != 0) printf("\n");
return;
}

/*
 * Import statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if(AddModule(tokens[1]) > 0) {		/* load module */
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

currentfunction->stat |= IF_STATEMENT;

printf("ifptr=%lX\n",currentptr);

while(*currentptr != 0) {

 if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {  
  exprtrue=EvaluateCondition(tokens,1,tc-1);

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

		tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
		if(tc == -1) {
		 PrintError(SYNTAX_ERROR);
		 return(-1);
		}

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

		tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
		if(tc == -1) {
		 PrintError(SYNTAX_ERROR);
		 return(-1);
		}

		if(strcmpi(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
 }
}

 currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
 tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */

 if(tc == -1) {
  PrintError(SYNTAX_ERROR);
  return(-1);
 }

}

//PrintError(ENDIF_NOIF);
}

/*
 * Endif statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
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


currentfunction->stat |= FOR_STATEMENT;

/* find end of variable name */

for(count=1;count<tc;count++) {
 if(strcmpi(tokens[count],"=") == 0) break;
}

if(count == tc) {		/* no = */
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
 steppos=doexpr(tokens,countx+1,tc);
}

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

PushSaveInformation();					/* save line information */

 while((ifexpr == 1 && loopcount.d > exprtwo) || (ifexpr == 0 && loopcount.d < exprtwo)) {
	    currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */	

	    ExecuteLine(buf);

	     d=*buf+(strlen(buf)-1);
             if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
             if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

 	     tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	     if(tc == -1) {
		 PrintError(SYNTAX_ERROR);
		 return(-1);
	     }


  	     if(strcmpi(tokens[0],"NEXT") == 0) {

	      info=currentfunction->saveinformation_top;
	      currentptr=info->bufptr;
	      currentfunction->lc=info->lc;
	
	      if(ifexpr == 1) loopcount.d=loopcount.d-steppos;					/* increment or decrement counter */
 	      if(ifexpr == 0) loopcount.d=loopcount.d+steppos;      
		
	      UpdateVariable(split.name,&loopcount,split.x,split.y);			/* set loop variable to next */	

              if(*currentptr == 0) {
	       PopSaveInformation();               
	       currentfunction->stat &= FOR_STATEMENT;

	       PrintError(SYNTAX_ERROR);
	       return(SYNTAX_ERROR);
              }
	    }	   
     }

   return;
 }

/*
 * Return statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

extern int callpos;

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 int count;
 int vartype;

 currentfunction->stat &= FUNCTION_STATEMENT;

/* check return type */

 for(count=1;count<tc;count++) {
  if((*tokens[count] >= '0' && *tokens[count] <= '9') && (currentfunction->returntype == VAR_STRING)) {
   PrintError(TYPE_ERROR);
   return(-1);
  }

  if((*tokens[count] == '"') && (currentfunction->returntype != VAR_STRING)) {
   PrintError(TYPE_ERROR);
   return(-1);
  }

  vartype=GetVariableType(tokens[count]);

  if(vartype != -1) {
   if((currentfunction->returntype == VAR_STRING) && (vartype != VAR_STRING)) {
    PrintError(TYPE_ERROR);
    return(-1);
   }

   if((currentfunction->returntype != VAR_STRING) && (vartype == VAR_STRING)) {
    PrintError(TYPE_ERROR);
    return(-1);
   }

 }
}


 retval.type=currentfunction->returntype;		/* get return type */

 if(currentfunction->returntype == VAR_STRING) {		/* returning string */
  ConatecateStrings(1,tc,tokens,&retval);		/* get strings */
  return;
 }
 else if(currentfunction->returntype == VAR_INTEGER) {		/* returning integer */
  retval.i=doexpr(tokens,1,tc);
 }
 else if(currentfunction->returntype == VAR_NUMBER) {		/* returning double */	 
	 retval.d=doexpr(tokens,1,tc);
 }
 else if(currentfunction->returntype == VAR_SINGLE) {		/* returning single */
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
  PrintError(NEXT_WITHOUT_FOR);
  return;
 }
}

/*
 * While statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
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
	tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
	if(tc == -1) {
	 PrintError(SYNTAX_ERROR);
	 return(-1);
	}

        if(strcmpi(tokens[0],"WEND") == 0) {
         PopSaveInformation();               
         currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */
	 return;
        }
       }

      }

      tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
       if(tc == -1) {
	 PrintError(SYNTAX_ERROR);
	 return(-1);
     }

      if(strcmpi(tokens[0],"WEND") == 0) {
       info=currentfunction->saveinformation_top;
       currentptr=info->bufptr;
       currentfunction->lc=info->lc;
      }

     ExecuteLine(buf);
  } while(exprtrue == 1);
  

PopSaveInformation();               
}

/*
 * End statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
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
 */

int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat& IF_STATEMENT) != IF_STATEMENT) {
  PrintError(ELSE_WITHOUT_IF);
  return(ELSE_WITHOUT_IF);
 }

return;
}

/*
 * Elseif statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 */

int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat& IF_STATEMENT) != IF_STATEMENT) {
  PrintError(ELSE_WITHOUT_IF);
  return(ELSE_WITHOUT_IF);
 }

return;
}
/*
 * Endfunction statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
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
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 char *buf[MAX_SIZE];
 char *d;

 if((strcmpi(tokens[1],"FOR") == 0) && (currentfunction->stat & FOR_STATEMENT)){
  PrintError(EXIT_FOR_WITHOUT_FOR);
  return(EXIT_FOR_WITHOUT_FOR);
 }

 if((strcmpi(tokens[1],"WHILE") == 0) && (currentfunction->stat & WHILE_STATEMENT)){
  PrintError(EXIT_WHILE_WITHOUT_WHILE);
  return(EXIT_WHILE_WITHOUT_WHILE);
 }

/* find end of loop */
 while(*currentptr != 0) {
   currentptr=ReadLineFromBuffer(currentptr,buf,MAX_SIZE);			/* get data */
   
   if(*currentptr == 0) {			/* at end without next or wend */   

	if((strcmpi(tokens[1],"FOR") == 0) && (currentfunction->stat & FOR_STATEMENT)){
  		PrintError(FOR_WITHOUT_NEXT);
		return(FOR_WITHOUT_NEXT);
	}

	if((strcmpi(tokens[1],"WHILE") == 0) && (currentfunction->stat & WHILE_STATEMENT)){
  		PrintError(WHILE_WITHOUT_WEND);
		return(WHILE_WITHOUT_WEND);
	}
   }

   tc=TokenizeLine(buf,tokens,TokenCharacters);			/* tokenize line */
   if(tc == -1) {
	    PrintError(SYNTAX_ERROR);
	    return(-1);
   }

   if((strcmpi(tokens[1],"NEXT") == 0) && (currentfunction->stat & FOR_STATEMENT)){
  		currentfunction->stat &= FOR_STATEMENT;
		return;
   }

   if((strcmpi(tokens[1],"WEND") == 0) && (currentfunction->stat & WHILE_STATEMENT)){
  		currentfunction->stat &= WHILE_STATEMENT;
		return;
   }

 } 
}

/*
 * Declare statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error number on error or 0 on success
 *
 */

int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 varsplit split;
 int vartype;
 int count;
 int retval;

 ParseVariableName(tokens,1,tc,&split);

 for(count=0;count<tc;count++) {  
  if(strcmpi(tokens[count],"AS") == 0) {		/* array as type */
   vartype=CheckVariableType(tokens[count+1]);		/* get variable type */  

   break;
  }
 }

 if(vartype == -1) {				/* invalid variable type */
  PrintError(BAD_TYPE);
  return(-1);
 }

 retval=CreateVariable(split.name,vartype,split.x,split.y);

 if(retval != NO_ERROR) {
  PrintError(retval);
 }


}

/*
 * Continue statement
 *
 * In: int tc				Token count
       char *tokens[MAX_SIZE][MAX_SIZE]	Tokens array
 *
 * Returns error on error or 0 on success
 *
 */

int iterate_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
SAVEINFORMATION *info;

info=currentfunction->saveinformation_top;
info->bufptr=currentptr;
info->lc=currentfunction->lc;

 if(((currentfunction->stat & FOR_STATEMENT)) || ((currentfunction->stat & WHILE_STATEMENT))) {
  info=currentfunction->saveinformation_top;  
  currentptr=info->bufptr;
  return(0);
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
 memset(tokens,0,10*MAX_SIZE);				/* clear line */
 
 d=tokens[0];

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
    
     if(*token == ' ') {
      d=tokens[++tc]; 

      token++;
      break;
     }
     else
     {
      b=token;
      b--;
      if(strlen(tokens[tc]) != 0) tc++;
     
      d=tokens[tc]; 			
      *d=*token++;
      d=tokens[++tc]; 		

      IsSeperator=TRUE;    
     }

     break;
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

do {
 if(statements[statementcount].statement == NULL) break;

 if(strcmpi(statements[statementcount].statement,token) == 0) return(TRUE);

} while(statements[statementcount++].statement != NULL);

 return(FALSE);
}

/*
 * Check syntax
 *
 * In: tokens		Tokens to check
       separators	Seperator characters to check against
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

 return(TRUE);

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

 for(count=start;count<end;count++) {

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
 * Display error
 *
 * In: int llcerr			Error number
 *
 * Returns: Nothing
 *
 */

int PrintError(int llcerr) {
 if(GetInteractiveModeFlag() == TRUE) {
  printf("Error in function %s: %s\n",currentfunction->name,llcerrs[llcerr]);  
 }
 else
 {
  printf("Error in function %s (line %d): %s\n",currentfunction->name,currentfunction->lc,llcerrs[llcerr]);
  exit(llcerr);
 }
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

int PushSaveInformation(void) {
SAVEINFORMATION *info;
SAVEINFORMATION *previousinfo;

if(currentfunction->saveinformation_top == NULL) {
 currentfunction->saveinformation_top=malloc(sizeof(SAVEINFORMATION));		/* allocate new entry */

 if(currentfunction->saveinformation_top == NULL) {
  PrintError(NO_MEM);
  return(-1);
 }

 info=currentfunction->saveinformation_top;
 info->last=NULL;
}
else
{
 previousinfo=currentfunction->saveinformation_top;		/* point to top */

 previousinfo->next=malloc(sizeof(SAVEINFORMATION));		/* allocate new entry */
 if(previousinfo->next == NULL) {
  PrintError(NO_MEM);
  return(-1);
 }

 info=previousinfo->next;
 info->last=previousinfo;					/* link previous to next */
}

info->bufptr=currentptr;
info->lc=currentfunction->lc;
info->next=NULL;

currentfunction->saveinformation_top=info;
}

int PopSaveInformation(void) {
SAVEINFORMATION *info;
SAVEINFORMATION *previousinfo;

info=currentfunction->saveinformation_top;

previousinfo=info;
currentfunction->saveinformation_top=info->last;		/* point to previous */

free(previousinfo);

currentptr=currentfunction->saveinformation_top->bufptr;
currentfunction->lc=currentfunction->saveinformation_top->lc;

}

void InteractiveMode(void) {
char *tokens[MAX_SIZE][MAX_SIZE];
char *endstatement[MAX_SIZE];
int block_statement_nest_count=0;
char *b;
char *linebuf[MAX_SIZE];
char *buffer;
char *bufptr;
int statementcount;

SetInteractiveModeFlag(TRUE);

memset(CurrentFile,0,MAX_SIZE);

buffer=malloc(INTERACTIVE_BUFFER_SIZE);		/* allocate buffer for interactive mode */
if(buffer == NULL) {
  perror("xscript");
  exit(NO_MEM);
}

bufptr=buffer;
currentptr=buffer;

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

 statementcount=0;

 do {
   if(statements[statementcount].statement == NULL) break;
 
   if(strcmpi(statements[statementcount].statement,tokens[0]) == 0) {  

    if(statements[statementcount].is_block_statement == TRUE) {

      strcpy(endstatement,statements[statementcount].endstatement);
      block_statement_nest_count++;      
    }

    break;
   }

   statementcount++;
  
 } while(statements[statementcount].statement != NULL);

      
 if(strcmp(tokens[0],endstatement) == 0) {
    block_statement_nest_count--;

    bufptr=buffer;
 }
  
 if(block_statement_nest_count == 0) {
   bufptr=buffer;

   do {
    currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */	

    ExecuteLine(bufptr);

    bufptr += strlen(bufptr);

  } while(*bufptr != 0);

    memset(buffer,0,INTERACTIVE_BUFFER_SIZE);

    bufptr=buffer;
    currentptr=buffer;

 }
 else
 {
   bufptr += strlen(bufptr);
 }

 } 
}

int GetInteractiveModeFlag(void) {
 return((Flags & INTERACTIVE_MODE_FLAG));
}

void SetInteractiveModeFlag(void) {
  Flags |= INTERACTIVE_MODE_FLAG;
}

void SetIsRunningFlag(void) {
  Flags |= IS_RUNNING_FLAG;
}

int GetIsRunningFlag(void) {
 return((Flags & IS_RUNNING_FLAG));
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
 if(strlen(CurrentFile) == 0) {
  	printf("No file loaded\n");
	return;
 }

 ExecuteFile(CurrentFile);
}

int single_step_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 int StepCount=0;
 int count;
 char *linebuf[MAX_SIZE];

 if(strlen(tokens[1]) > 0) {
	StepCount=doexpr(tokens,1,tc);
 }
 else
 {
	StepCount=1;
 }

 for(count=0;count<StepCount;count++) {
	 currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

	 if(*currentptr == 0) {
		printf("End reached\n");
		return;
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
	return;
 }

 printf("Invalid sub-command\n");
 return;
}

int clear_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(tc < 2) {						/* Not enough parameters */
  PrintError(SYNTAX_ERROR);
  return(SYNTAX_ERROR);
 }

 if(strcmpi(tokens[1],"BREAKPOINT") == 0) {		/* set breakpoint */
	clear_breakpoint(atoi(tokens[2]),tokens[3]);
	return;
 }

 printf("Invalid sub-command\n");
 return;
}
