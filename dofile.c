/*
 * do file
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "define.h"
#include "defs.h"
char *llcerrs[] = { "No error","File not found","No parameters for statement","Bad expression",\
		    "IF statement without ELSEIF or ENDIF","FOR statement without NEXT",\
		    "WHILE without WEND","ELSE without IF","ENDIF without IF","ENDFUNCTION without FUNCTION",\
		    "Invalid variable name","Out of memory","BREAK outside FOR or WHILE loop","Read error","Syntax error",\
		    "Error calling library function","Invalid statement","Nested function","ENDFUNCTION without FUNCTION",\
		    "NEXT without FOR","WEND without WHILE","Duplicate function","Too few arguments",\
		    "Invalid array subscript","Type mismatch","Invalid type","CONTINUE without FOR or WHILE","ELSEIF without IF",\
		    "Invalid condition","Invalid type in declaration" };

char *readlinefrombuffer(char *buf,char *linebuf,int size);

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
int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);

int saveexprtrue=0;
extern int substitute_vars(int start,int end,char *tokens[][MAX_SIZE]);
varval retval;

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
      { "RETURNS",&bad_keyword_as_statement },\
      { "DOUBLE",&bad_keyword_as_statement },\
      { "STRING",&bad_keyword_as_statement },\
      { "INTEGER",&bad_keyword_as_statement },\
      { "SINGLE",&bad_keyword_as_statement },\
      { NULL,NULL } };

extern functions *currentfunction;
extern functions *funcs;
extern char *vartypenames[];

char *currentptr=NULL;
char *endptr=NULL;
char *readbuf=NULL;
int bufsize=0;
int ic=0;

MODULES *modules=NULL;
include includefiles[MAX_INCLUDE];

int loadfile(char *filename) {
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
   print_error(NO_MEM);
   return(NO_MEM);
  }
 }
 else
 {
  if(realloc(readbuf,bufsize+filesize) == NULL) {
   print_error(NO_MEM);
   return(NO_MEM);
  }
 }

 currentfunction->saveinformation[0].bufptr=readbuf;
 currentfunction->saveinformation[0].lc=0;
 currentfunction->nestcount=0;

 currentptr=readbuf;

 if(fread(readbuf,filesize,1,handle) != 1) {		/* read to buffer */
  print_error(READ_ERROR);
  return(READ_ERROR);
 }


 strcpy(includefiles[ic].filename,filename);
		
 endptr += filesize;		/* point to end */
 bufsize += filesize;

}

int dofile(char *filename) {
 char *linebuf[MAX_SIZE];

 //printf("debug\n");

 includefiles[ic].lc=0;
 
 if(loadfile(filename) == -1) {
  print_error(FILE_NOT_FOUND);
  return(FILE_NOT_FOUND);
}

do {
 currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;

 currentptr=readlinefrombuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

 currentfunction->saveinformation[currentfunction->nestcount].lc=includefiles[ic].lc;;

 if(*currentptr == 0) return; 

 doline(linebuf);

 memset(linebuf,0,MAX_SIZE);

 includefiles[ic].lc++;
}    while(*currentptr != 0); 			/* until end */

 return;
}	

/* 
 * Process line
*
 */

int doline(char *lbuf) {
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

tc=tokenize_line(lbuf,tokens," \009");			/* tokenize line */

/* do statement */

statementcount=0;

do {
 if(statements[statementcount].statement == NULL) break;

// printf("%s %s\n",statements[statementcount].statement,tokens[0]);

 if(strcmpi(statements[statementcount].statement,tokens[0]) == 0) {  
  if(statements[statementcount].call_statement(tc,tokens) == -1) exit(-1);		/* call statement and exit if error */
  statementcount=0;

  return;
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


if(check_function(functionname) != -1) {	/* user function */
 if(callfunc(functionname,args) == -1) exit(-1);
} 


/*
 *
 * assignment
 *
 */
for(count=1;count<tc;count++) {

 if(strcmpi(tokens[count],"=") == 0) {

	 splitvarname(tokens[count-1],&split);			/* split variable */

	 vartype=getvartype(split.name);

	 c=*tokens[count+1];

	 if((c == '"') || getvartype(split.name) == VAR_STRING) {			/* string */  
	  if(vartype == -1) {
		addvar(split.name,VAR_STRING,split.x,split.y);		/* new variable */ 
	  }
	  else
	  {  
	   print_error(TYPE_ERROR);
	   return(TYPE_ERROR);
	  }

	  conatecate_strings(count+1,tc,tokens,&val);					/* join all the strings on the line */

	  updatevar(split.name,&val,split.x,split.y);		/* set variable */

	  return;
	 }

	/* number otherwise */

	 if(vartype == VAR_STRING) {		/* not string */
	  print_error(TYPE_ERROR);
	  return(TYPE_ERROR);
	 }

	 exprone=doexpr(tokens,count+1,tc);

	 if(vartype == VAR_NUMBER) {
	  val.d=exprone;
	 }
 	 else if(vartype == VAR_STRING) {
	  substitute_vars(count+1,count+1,tokens); 
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
	  addvar(split.name,VAR_NUMBER,split.x,split.y);			/* create variable */
	  updatevar(split.name,&val,split.x,split.y);
	  return;
	 }

	 updatevar(split.name,&val,split.x,split.y);

	 return;
  } 

}

print_error(INVALID_STATEMENT);

return;
}



/*
 * declare function
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

b=tokens[1];		/* get function name */
d=functionname;

while(*b != 0) {	/* copy until ) */
 if(*b == '(') {
  b++;
  break;
 }

 *d++=*b++;
}

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
   print_error(BAD_TYPE);
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

if((strlen(tokens[2]) > 0) && (strcmpi(tokens[2],"RETURNS") != 0)) {			/* not returns */
  print_error(SYNTAX_ERROR);
  return;
}

if(strcmpi(tokens[2],"RETURNS") == 0) {			/* return type */
 vartype=check_var_type(tokens[3]);		/* get variable type */  

 if(vartype == -1) {				/* invalid variable type */
  print_error(BAD_TYPE);
  return(-1);
 }
}
else
{
 vartype=VAR_NUMBER;
}

function(functionname,args,vartype);
return;
} 

/*
 * display message
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
double exprone;
char c;
varval val;
int countx;
varsplit split;

splitvarname(tokens[1],&split);


c=*tokens[1];

/* if string literal, string variable or function returning string */

if((c == '"') || (getvartype(tokens[1]) == VAR_STRING) || (check_function(split.name) == VAR_STRING) ) {	/* user function */ 
 conatecate_strings(1,tc,tokens,&val);					/* join all the strings on the line */
 
 printf("%s\n",val.s);
 return;
}

exprone=doexpr(tokens,1,tc);
printf("%.6g ",exprone);

printf("\n");
return;
}

/*
 *
 * import library
 */

int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;
MODULES *next;
MODULES *last;
int handle;

handle=open_module(tokens[1]);
if(handle == -1) {	/* can't open module */
 print_error(FILE_NOT_FOUND);
 return(-1);
}

if(modules == NULL) {			/* first in list */
 modules=malloc(sizeof(MODULES));
 if(modules == NULL) {
  print_error(NO_MEM);
  return(-1);
 }

 last=modules;
}
else
{
 next=modules;

 while(next != NULL) {
  last=next;
  next=next->next;
 }
}

 last->next=malloc(sizeof(MODULES));
 if(modules == NULL) {
  print_error(NO_MEM);
  return(-1);
 }

 next=last->next;
 strcpy(next->modulename,tokens[1]);
 next->dlhandle=handle;
return;
}

/*
 * IF statement
 *
 */

int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
int count;
int countx;
char *d;
int exprtrue;

if(tc < 1) {						/* not enough parameters */
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

currentfunction->stat |= IF_STATEMENT;

while(*currentptr != 0) {

if((strcmpi(tokens[0],"IF") == 0) || (strcmpi(tokens[0],"ELSEIF") == 0)) {
  exprtrue=do_condition(tokens,1,tc-1);
  if(exprtrue == -1) {
   print_error(BAD_CONDITION);
   return(-1);
  }

if(exprtrue == 1) {
		saveexprtrue=exprtrue;

		do {
    		currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
		doline(buf);

		tokenize_line(buf,tokens," \009");			/* tokenize line */

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
    		currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

		doline(buf);

		tokenize_line(buf,tokens," \009");			/* tokenize line */

		if(strcmpi(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmpi(tokens[0],"ENDIF") != 0) && (strcmpi(tokens[0],"ELSEIF")) != 0);
 }
}

 currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
 tokenize_line(buf,tokens," \009");			/* tokenize line */

}

//print_error(ENDIF_NOIF);
}

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & IF_STATEMENT) == 0) print_error(ENDIF_NOIF);
}

/*
 * loop statement
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
 print_error(NO_PARAMS);
 return(NO_PARAMS);
}

currentfunction->stat |= FOR_STATEMENT;

if(strcmpi(tokens[2],"=") != 0) {
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

//  0  1     2 3 4  5
// for count = 1 to 10

for(count=1;count<tc;count++) {
 if(strcmpi(tokens[count],"TO") == 0) break;		/* found to */
}

if(count == tc) {
 print_error(SYNTAX_ERROR);
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

splitvarname(tokens[1],&split);

//  0   1    2 3 4  5
// for count = 1 to 10

exprone=doexpr(tokens,3,count);			/* start value */
exprtwo=doexpr(tokens,count+1,countx);			/* end value */

if(getvarval(split.name,&loopx) == -1) {		/* new variable */  
 addvar(split.name,VAR_NUMBER,split.x,split.y);
}

vartype=getvartype(split.name);			/* check if string */

switch(vartype) {
 case VAR_STRING: 
  print_error(TYPE_ERROR);
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

updatevar(split.name,&loopcount,split.x,split.y);			/* set loop variable to next */	

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
	    currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */	

	    doline(buf);

	     d=*buf+(strlen(buf)-1);
             if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
             if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

 	     tokenize_line(buf,tokens," \009");			/* tokenize line */

  	     if(strcmpi(tokens[0],"NEXT") == 0) {

	      includefiles[ic].lc;currentfunction->saveinformation[currentfunction->nestcount].lc;

	      currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;		/* restore position */   	    

			
	      if(ifexpr == 1) loopcount.d=loopcount.d-steppos;					/* increment or decrement counter */
 	      if(ifexpr == 0) loopcount.d=loopcount.d+steppos;      
		
	      updatevar(split.name,&loopcount,split.x,split.y);			/* set loop variable to next */	

              if(*currentptr == 0) {
	          if(currentfunction->nestcount-1 > 0) currentfunction->nestcount--;
               
	       currentfunction->stat &= FOR_STATEMENT;
	       print_error(SYNTAX_ERROR);
	       return(SYNTAX_ERROR);
              }

		
	    }	   
     }

  if(currentfunction->nestcount-1 > 0) currentfunction->nestcount--;
  currentfunction->stat &= FOR_STATEMENT;
  return;
 }

/*
 * return from call
 */

extern int callpos;

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 int count;
 int vartype;

 currentfunction->stat &= FUNCTION_STATEMENT;

/* check return type */

 for(count=1;count<tc;count++) {
  if((*tokens[count] >= '0' && *tokens[count] <= '9') && (currentfunction->return_type == VAR_STRING)) {
   print_error(TYPE_ERROR);
   return(-1);
  }

  if((*tokens[count] == '"') && (currentfunction->return_type != VAR_STRING)) {
   print_error(TYPE_ERROR);
   return(-1);
  }

  vartype=getvartype(tokens[count]);

  if(vartype != -1) {
   if((currentfunction->return_type == VAR_STRING) && (vartype != VAR_STRING)) {
    print_error(TYPE_ERROR);
    return(-1);
   }

   if((currentfunction->return_type != VAR_STRING) && (vartype == VAR_STRING)) {
    print_error(TYPE_ERROR);
    return(-1);
   }

 }
}

 retval.type=currentfunction->return_type;

 if(currentfunction->return_type == VAR_STRING) {		/* returning string */
  conatecate_strings(1,tc,tokens,&retval);		/* get strings */
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

return_from_function();			/* return */
 return;
}

int get_return_value(varval *val) {
 val=&retval;
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & WHILE_STATEMENT) == 0) print_error(WEND_NOWHILE);
 return;
}

int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & FOR_STATEMENT) == 0) {
  print_error(NEXT_NO_FOR);
  return;
 }
}

/* 
 * WHILE statement
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
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

//asm("int $3");

currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;
currentfunction->saveinformation[currentfunction->nestcount].lc=includefiles[ic].lc;

memcpy(condition_tokens,tokens,(tc*(MAX_SIZE*MAX_SIZE)));		/* save copy of condition */
condition_tc=tc;

currentfunction->stat |= WHILE_STATEMENT;
 
substitute_vars(1,condition_tc,condition_tokens);
exprtrue=do_condition(condition_tokens,1,condition_tc);			/* do condition */
if(exprtrue == -1) {
 print_error(BAD_CONDITION);
 return(-1);
}

do {
      currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

      exprtrue=do_condition(condition_tokens,1,condition_tc);			/* do condition */
      if(exprtrue == -1) {
       print_error(BAD_CONDITION);
       return(-1);
      }

      if(exprtrue == 0) {
       while(*currentptr != 0) {

        currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
        tc=tokenize_line(buf,tokens," \009");

        if(strcmpi(tokens[0],"WEND") == 0) {
         currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
	 return;
        }
       }

      }

      tc=tokenize_line(buf,tokens," \009");

      if(strcmpi(tokens[0],"WEND") == 0) {
       includefiles[ic].lc;currentfunction->saveinformation[currentfunction->nestcount].lc;
       currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;
       return;
      }

       doline(buf);
  } while(exprtrue == 1);
  

}

/* end program */
	
int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 return(atoi(tokens[1]));
}

/*
 *
 * else
 *
 */

int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat& IF_STATEMENT) != IF_STATEMENT) {
  print_error(ELSE_NOIF);
  return(ELSE_NOIF);
 }

return;
}

int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
if((currentfunction->stat & FUNCTION_STATEMENT) != FUNCTION_STATEMENT) {
 print_error(ENDFUNCTION_NO_FUNCTION);
 return(ENDFUNCTION_NO_FUNCTION);
}

currentfunction->stat|= FUNCTION_STATEMENT;
return;
}

int include_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(loadfile(tokens[1]) == -1) {
 print_error(FILE_NOT_FOUND);
 return(FILE_NOT_FOUND);
}
 
}

int break_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 char *buf[MAX_SIZE];
 char *d;

 if(((currentfunction->stat & FOR_STATEMENT) == 0) && ((currentfunction->stat & WHILE_STATEMENT) == 0)) {
  print_error(INVALID_BREAK);
  return(INVALID_BREAK);
 }

 if((currentfunction->stat & FOR_STATEMENT) || (currentfunction->stat & WHILE_STATEMENT)) {
  while(*currentptr != 0) {
   currentptr=readlinefrombuffer(currentptr,buf,MAX_SIZE);			/* get data */
   
   if(*currentptr == 0) {			/* at end without next or wend */   
    if((currentfunction->stat & FOR_STATEMENT) == 0) {
    print_error(FOR_NO_NEXT);
    return(FOR_NO_NEXT);
   }

    if((currentfunction->stat & WHILE_STATEMENT) == 0) {
    print_error(WEND_NOWHILE);
    return(WEND_NOWHILE);
   }
  }

   tokenize_line(buf,tokens," \009");

   if((strcmpi(tokens[0],"WEND") == 0) || (strcmpi(tokens[0],"NEXT") == 0)) {
    if((strcmpi(tokens[0],"WEND") == 0)) currentfunction->stat &= WHILE_STATEMENT;
    if((strcmpi(tokens[0],"NEXT") == 0)) currentfunction->stat &= FOR_STATEMENT;
  
    currentptr=readlinefrombuffer(currentptr,buf,MAX_SIZE);			/* get data */
    return;
   }
  }
 } 
}

int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 varsplit split;
 int vartype;

 splitvarname(tokens[1],&split);

 if(strcmpi(tokens[2],"AS") == 0) {		/* array as type */
  vartype=check_var_type(tokens[3]);		/* get variable type */  
 
  if(vartype == -1) {				/* invalid variable type */
   print_error(BAD_TYPE);
   return(-1);
  }
 }
 else
 {
  vartype=VAR_NUMBER;
 }

 addvar(split.name,vartype,split.x,split.y);

}

int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(dofile(tokens[1]) == -1) {
  print_error(FILE_NOT_FOUND);
  return(FILE_NOT_FOUND);
 }

}

int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(((currentfunction->stat & FOR_STATEMENT)) || ((currentfunction->stat & WHILE_STATEMENT))) {
  currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;
  return;
 }

 print_error(CONTINUE_NO_LOOP);
 return(CONTINUE_NO_LOOP);
}

int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 print_error(SYNTAX_ERROR);
}

int tokenize_line(char *linebuf,char *tokens[][MAX_SIZE],char *split) {
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

char *readlinefrombuffer(char *buf,char *linebuf,int size) {
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

int print_error(int llcerr) {
 if(*includefiles[ic].filename == 0) {			/* if in interactive mode */
  printf("%s\n",llcerrs[llcerr]);
 }
 else
 {
  printf("%s %d: %s\n",includefiles[ic].filename,includefiles[ic].lc,llcerrs[llcerr]);
 }
}

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
