/*
 * do file
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"
#include "defs.h"
char *llcerrs[] = { "No error","Missing label","File not found","No parameters for statement","Bad expression",\
		    "IF statement without ELSEIF or ENDIF","FOR statement without NEXT",\
		    "WHILE without WEND","ELSE without IF","ENDIF without IF","ENDFUNCTION without FUNCTION",\
		    "Invalid variable name","Out of memory","EXIT FOR without FOR","Read error","Syntax error",\
		    "Error calling library function","Invalid statement","Nested function","ENDFUNCTION without FUNCTION",\
		    "NEXT without FOR","WEND without WHILE","Duplicate function","Too few arguments","EXIT LOOP without WHILE",
		    "Invalid array subscript","Type mismatch","Invalid type","CONTINUE without FOR or WHILE","ELSEIF without IF" };

char *readlinefrombuffer(char *buf,char *linebuf,int size);

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int goto_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int sys_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
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
int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int dim_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int exprtrue=0;
extern int substitute_vars(int start,int end,char *tokens[][MAX_SIZE]);

statement statements[] = { { "IF",&if_statement },\
      { "ELSE",&else_statement },\
      { "ENDIF",&endif_statement },\
      { "FOR",&for_statement },\
      { "WHILE",&while_statement },\
      { "WEND",&wend_statement },\
      { "PRINT",&print_statement },\
      { "GOTO",&goto_statement },\
      { "SYS",&sys_statement },\ 
      { "END",&end_statement },\ 
      { "FUNCTION",&function_statement },\ 
      { "ENDFUNCTION",&endfunction_statement },\ 
      { "RETURN",&return_statement },\ 
      { "INCLUDE",&include_statement },\ 
      { "EXIT",&exit_statement },\
      { "DIM",&dim_statement },\
      { "RUN",&run_statement },\
      { "CONTINUE",&continue_statement },\
      { NULL,NULL } };

extern functions *currentfunction;
extern functions *funcs;
extern char *vartypenames[];

char *currentptr=NULL;
char *endptr=NULL;
char *readbuf=NULL;
int bufsize=0;
int ic=0;
int l;
int lc[MAX_SIZE];

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
   exit(NO_MEM);
  }
 }
 else
 {
  if(realloc(readbuf,bufsize+filesize) == NULL) {
   print_error(NO_MEM);
   exit(NO_MEM);
  }
 }

 currentfunction->saveinformation[0].bufptr=readbuf;
 currentfunction->saveinformation[0].lc=0;
 currentfunction->nestcount=0;

 currentptr=readbuf;

 if(fread(readbuf,filesize,1,handle) != 1) {		/* read to buffer */
  print_error(READ_ERROR);
  exit(READ_ERROR);
 }


 strcpy(includefiles[ic].filename,filename);
		
 endptr += filesize;		/* point to end */
 bufsize += filesize;

}

int dofile(char *filename) {
 char *linebuf[MAX_SIZE];

 //printf("debug\n");

 lc[l]=0;
 
 if(loadfile(filename) == -1) {
  print_error(MISSING_FILE);
  exit(MISSING_FILE);
}

do {
 currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;

 currentptr=readlinefrombuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

 currentfunction->saveinformation[currentfunction->nestcount].lc=lc[l];

 if(*currentptr == 0) {
  free(readbuf);
  return; 
 }

 doline(linebuf);

 memset(linebuf,0,MAX_SIZE);

 lc[l]++;
}    while(*currentptr != 0); 			/* until end */

 
 free(readbuf);
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
 char *functionname[MAX_SIZE];
 char *args[MAX_SIZE];
 char *b;
 char *d;

 lc[l]++;						/* increment line counter */

 /* return if blank line */

 c=*lbuf;

if(c == '\r' || c == '\n' || c == 0) return;			/* blank line */

if(*(lbuf+(strlen(lbuf)-1)) == '\n') *(lbuf+(strlen(lbuf)-1))=0;	/* remove newline from line if found */
if(*(lbuf+(strlen(lbuf)-1)) == '\r') *(lbuf+(strlen(lbuf)-1))=0;	/* remove newline from line if found */

while(*lbuf == ' ' || *lbuf == '\t') lbuf++;	/* skip white space */

if(memcmp(lbuf,"//",2) == 0) return;		/* skip comments */

memset(tokens,0,MAX_SIZE*MAX_SIZE);

tc=tokenize_line(lbuf,tokens," \009");			/* tokenize line */

if(*lbuf+(strlen(lbuf)-1) == ':') return;		/* label */

/*
 *
 * assignment
 *
 */

if(strcmp(tokens[1],"=") == 0) {
 splitvarname(tokens[0],&split);			/* split variable */

 vartype=getvartype(split.name);

 c=*tokens[2];

 if(c == '"') {			/* string */  
  if(vartype == -1) addvar(split.name,VAR_STRING,split.x,split.y);		/* new variable */ 
  
  if(vartype != VAR_STRING) {		/* not string */
   print_error(TYPE_ERROR);
   exit(TYPE_ERROR);
  }

  strcpy(val.s,tokens[2]);  
  updatevar(split.name,&val,split.x,split.y);		/* set variable */

  return;
 }

/* number otherwise */

 if(vartype == VAR_STRING) {		/* not string */
  print_error(TYPE_ERROR);
  exit(TYPE_ERROR);
 }

 exprone=doexpr(tokens,2,tc);
 
 val.d=exprone;

 if(vartype == -1) {		/* new variable */ 
  addvar(split.name,VAR_NUMBER,split.x,split.y);			/* create variable */
  updatevar(split.name,&val,split.x,split.y);
  return(0);
 }

 updatevar(split.name,&val,split.x,split.y);

 return;
} 

/* do statement */

statementcount=0;

do {
 if(statements[statementcount].statement == NULL) break;
 touppercase(tokens[0]);

// printf("%s %s\n",statements[statementcount].statement,tokens[0]);

 if(strcmp(statements[statementcount].statement,tokens[0]) == 0) {  
  statements[statementcount].call_statement(tc,tokens);
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
if(*d == ')') *d=0;	// no )


if(check_function(functionname) == 0) {	/* user function */
 callfunc(functionname,args);
} 
else
{
 print_error(INVALID_STATEMENT);
}

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

if(tc < 2) {						/* Not enough parameters */
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

b=tokens[1];		/* get function name */
d=functionname;

while(*b != 0) {
 if(*b == '(') {
  d=args;		/* copy to args */
  b++;
 }

 *d++=*b++;
}

*d++=' ';

for(count=2;count<tc;count++) {
 touppercase(tokens[count]);

 b=tokens[count];		/* get function name */
 
 while(*b != 0) *d++=*b++;

 *d++=' ';
}

d -= 2;

if(*d == ')') *d=0;

function(functionname,args);
return;
}
 

/*
 * display message
 *
 */

int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;
char *val;
int countx;
double exprone;
char *buf[MAX_SIZE];
int countz;
char *valptr;
int printtc;
char *printargs[10][MAX_SIZE];
char *printtokens[10][MAX_SIZE];
int parttc;
char c;
	
memset(buf,0,MAX_SIZE);

for(count=1;count != tc;count++) {				/* get print tokens */
 strcat(buf,tokens[count]);

 if((count != tc-1) && (*buf != ',')) strcat(buf," ");
}

memset(printargs,0,10*MAX_SIZE);
memset(printtokens,0,10*MAX_SIZE);

printtc=tokenize_line(buf,printargs,",");			/* copy args */
substitute_vars(0,printtc,printargs);

for(count=0;count < printtc;count++) {
 c=*printargs[count];
 
 if(c == '"' || (getvartype(printargs[count]) == VAR_STRING)) {
  valptr=printargs[count];
  valptr += (strlen(printargs[count])-2);
  
  *valptr=0;

  valptr=printargs[count];
  valptr++;

  printf("%s",valptr);
 }
 else
 {  
  parttc=tokenize_line(printargs[count],printtokens," ");
 
  exprone=doexpr(printtokens,0,parttc);
  printf("%.6g ",exprone);
 }

}

printf("\n");
return;
}

/*
 * goto
 *
 */

int goto_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
char *buf[MAX_SIZE];
char *d;

 d=*tokens[1]+(strlen(tokens[1])-1);
 if(*(tokens[1]+(strlen(tokens[1])-1)) == '\n') *d=0;	/* remove newline from line if found */
 if(*(tokens[1]+(strlen(tokens[1])-1)) == '\r') *d=0;	/* remove newline from line if found */ 

currentfunction->saveinformation[currentfunction->nestcount].bufptr=currentptr;
currentfunction->saveinformation[currentfunction->nestcount].lc=lc[l];

while(*currentfunction->saveinformation[currentfunction->nestcount].bufptr = 0) {
 currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

 if(*(buf+strlen(buf)-1) == ':') {
  d=*buf+(strlen(buf)-1);
  if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
  if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

  if(strcmp(buf,tokens[1]) == 0) return;	/* found label */ 
  }
 }

print_error(NO_GOTO);
return(NO_GOTO);
 }


/*
 *
 * call system/library function
 */

int sys_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
void  (*dladdr)(void);			/* function pointer */
void *dlhandle;
int count;

dlhandle=dlopen(tokens[0],RTLD_LAZY);			/* open library */

if(dlhandle == -1) {			/* can't open */
 print_error(MISSING_FILE);
 return(MISSING_FILE);
}

dladdr=dlsym(dlhandle,tokens[1]);		/* get address of library function */
if(dladdr == NULL) {					/* can't call function */
 print_error(MISSING_LIBSYM);
 return(MISSING_LIBSYM);
}

/* you are not expected to understand this */

dladdr=dlsym;		/* point to dlsym */

for(count=0;count<tc-1;count++) {
 asm volatile("push %0":: "b"(tokens[count]));
}

asm volatile("push %0":: "b"(dlhandle));
dladdr();
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

if(tc < 1) {						/* not enough parameters */
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

currentfunction->stat |= IF_STATEMENT;

while(*currentptr != 0) {

touppercase(tokens[0]);

printf("token=%s\n",tokens[0]);

exprtrue=do_condition(tokens,1,tc-1);

if((strcmp(tokens[0],"IF") == 0) || (strcmp(tokens[0],"ELSEIF") == 0)) {

  printf("exprtrue=%d\n",exprtrue);

  if(exprtrue == 1) {

		do {
    		currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

		printf("buf=%s\n",buf);
		doline(buf);

		tokenize_line(buf,tokens," \009");			/* tokenize line */

		touppercase(tokens[0]);

		if(strcmp(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmp(tokens[0],"ENDIF") != 0) && (strcmp(tokens[0],"ELSEIF")) != 0);
  }
}

 if((strcmp(tokens[0],"ELSE") == 0)) {
  if(exprtrue == 0) {
	    do {
    		currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

		printf("elsebuf=%s\n",buf);
		doline(buf);

		tokenize_line(buf,tokens," \009");			/* tokenize line */

		touppercase(tokens[0]);

		if(strcmp(tokens[0],"ENDIF") == 0) {
			currentfunction->stat |= IF_STATEMENT;
			return;
		}

	  } while((strcmp(tokens[0],"ENDIF") != 0) && (strcmp(tokens[0],"ELSEIF")) != 0);
 }
}

 currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
 tokenize_line(buf,tokens," \009");			/* tokenize line */

}

//print_error(ENDIF_NOIF);
}

int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 print_error(ENDIF_NOIF);
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

if(strcmp(tokens[2],"=") != 0) {
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

//  0  1     2 3 4  5
// for count = 1 to 10

for(count=1;count<tc;count++) {
 if(strcmp(tokens[count],"to") == 0) break;		/* found to */
}

if(count == tc) {
 print_error(SYNTAX_ERROR);
 return(SYNTAX_ERROR);
}

for(countx=1;countx<tc;countx++) {
 if(strcmp(tokens[countx],"step") == 0) break;		/* found step */
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
currentfunction->saveinformation[currentfunction->nestcount].lc=lc[l];

// printf("startptr=%lX\n",currentptr);

 while((ifexpr == 1 && loopcount.d > exprtwo) || (ifexpr == 0 && loopcount.d < exprtwo)) {
	    currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */	

	    doline(buf);

	     d=*buf+(strlen(buf)-1);
             if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
             if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

 	     tokenize_line(buf,tokens," \009");			/* tokenize line */

             touppercase(tokens[0]);
	
  	     if(strcmp(tokens[0],"NEXT") == 0) {

	      lc[l]=currentfunction->saveinformation[currentfunction->nestcount].lc;
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

int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 currentfunction->stat &= FUNCTION_STATEMENT;
 currentfunction->retval=doexpr(tokens,1,tc);				/* start value */	

 return;
}

int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if((currentfunction->stat & WHILE_STATEMENT) == 0) print_error(WEND_NOWHILE);
 return;
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

memcpy(condition_tokens,tokens,(tc*(MAX_SIZE*MAX_SIZE)));		/* save copy of condition */
condition_tc=tc;

currentfunction->stat |= WHILE_STATEMENT;
 
substitute_vars(1,condition_tc,condition_tokens);
exprtrue=do_condition(condition_tokens,1,condition_tc);			/* do condition */

do {
      currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

      exprtrue=do_condition(condition_tokens,1,condition_tc);			/* do condition */

      if(exprtrue == 0) {
       while(*currentptr != 0) {

        currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
        tc=tokenize_line(buf,tokens," \009");

        if(strcmp(tokens[0],"wend") == 0) {
         currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */
	 return;
        }
       }

      }


      d=*buf+(strlen(buf)-1);
      if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
      if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

      tc=tokenize_line(buf,tokens," \009");

      if(strcmp(tokens[0],"wend") == 0) {
       lc[l]=currentfunction->saveinformation[currentfunction->nestcount].lc;
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
 print_error(MISSING_FILE);
 return(MISSING_FILE);
}
 
}

int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 char *buf[MAX_SIZE];
 char *d;

 if((strcmp(tokens[1],"FOR") == 0) && ((currentfunction->stat & FOR_STATEMENT) == 0)) {
  print_error(EXITFOR_NO_FOR);
  exit(EXITFOR_NO_FOR);
 }

 if((strcmp(tokens[1],"LOOP") == 0) && ((currentfunction->stat & WHILE_STATEMENT) == 0)) {
  print_error(EXITWHILE_NO_LOOP);
  exit(EXITWHILE_NO_LOOP);
 }

 if((currentfunction->stat & FOR_STATEMENT) || (currentfunction->stat & WHILE_STATEMENT)) {
  while(*currentptr != 0) {
   currentptr=readlinefrombuffer(currentptr,buf,MAX_SIZE);			/* get data */
   
   if(*currentptr == 0) {			/* at end without next or wend */   
    print_error(EXITWHILE_NO_LOOP);
    return(EXITWHILE_NO_LOOP);
   }

   lc[l]++;

   d=*buf+(strlen(buf)-1);
   if(*(buf+(strlen(buf)-1)) == '\n') *d=0;	/* remove newline from line if found */
   if(*(buf+(strlen(buf)-1)) == '\r') *d=0;	/* remove newline from line if found */ 

   tokenize_line(buf,tokens," \009");

   touppercase(tokens[0]);           
   if((strcmp(tokens[0],"WEND") == 0) || (strcmp(tokens[0],"NEXT") == 0)) {
    if((strcmp(tokens[0],"WEND") == 0)) currentfunction->stat &= WHILE_STATEMENT;
    if((strcmp(tokens[0],"NEXT") == 0)) currentfunction->stat &= FOR_STATEMENT;
  
    currentptr=readlinefrombuffer(currentptr,buf,MAX_SIZE);			/* get data */
    return;
   }
  }
 } 
}

int dim_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 varsplit split;
 int count;

 splitvarname(tokens[1],&split);

 touppercase(tokens[2]);
 touppercase(tokens[3]);

 if(strcmp(tokens[2],"AS") == 0) {		/* array as type */
  count=0;

  while(vartypenames[count] != NULL) {
   touppercase(vartypenames[count]);
    if(strcmp(vartypenames[count],tokens[3]) == 0) break;	/* found type */
   
   count++;
  }

  if(vartypenames[count] == NULL) {		/* invalid type */
   print_error(BAD_TYPE);
   exit(BAD_TYPE);
  }
 }
 else
 {
  count=VAR_NUMBER;
 }

// printf("count=%d\n",count);

 addvar(split.name,count,split.x,split.y);

}

int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(dofile(tokens[1]) == -1) {
  print_error(MISSING_FILE);
  exit(MISSING_FILE);
 }

}

int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
 if(((currentfunction->stat & FOR_STATEMENT)) || ((currentfunction->stat & WHILE_STATEMENT))) {
  currentptr=currentfunction->saveinformation[currentfunction->nestcount].bufptr;
  return;
 }

 print_error(CONTINUE_NO_LOOP);
 exit(CONTINUE_NO_LOOP);
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
 if(*token == '"') {		/* quoted text */

   token++;
   *d++='"';
   while(*token != 0) {
    *d++=*token++;
    if(*d == '"') break;
   }

   *d++=0;  
   tc++;
 }
 else
 {
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

  if(*b == 0) break;
  if(*b == '\n') break;
} while(*b != 0);		/* until end of line */


b=linebuf;
b += strlen(linebuf);
b--;

*b=0;

return(buf);			/* return new position */
}

int print_error(int llcerr) {
 printf("FATAL ERROR %s %d: %s\n",includefiles[ic].filename,lc[l],llcerrs[llcerr]);
}

