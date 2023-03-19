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
 * Variables and functions
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"
#include "addvar.h"
#include "dofile.h"

extern varval retval;
functions *funcs=NULL;
functions *currentfunction=NULL;
record *records=NULL;

char *vartypenames[] = { "DOUBLE","STRING","INTEGER","SINGLE",NULL };

extern char *currentptr;
extern statement statements[];

struct {
 char *callptr;
 functions *funcptr;
 int lc;
} callstack[MAX_NEST_COUNT];

int callpos=0;

/*
 * Initalize function list
 *
 * In: Nothing
 *
 * Returns: -1 On error or 0 on success
 *
 */

extern char *TokenCharacters;

int InitializeFunctions(void) {
funcs=malloc(sizeof(funcs));		/* functions */
if(funcs == NULL) {
 PrintError(NO_MEM);
 return(-1);
}

memset(funcs,0,sizeof(funcs));

currentfunction=funcs;

callstack[0].funcptr=funcs;
strcpy(currentfunction->name,"main");		/* set function name */
return(0);
}

/*
 * Free function list
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */

void FreeFunctions(void) {
 free(funcs);

 return;
}

/*
 * Add variable
 *
 * In: char *name	Variable name
       int type		Variable type
       int xsize	Size of X subscript
       int ysize	Size of Y subscript
 *
 * Returns -1 on error or 0 on success
 *
 */

int CreateVariable(char *name,int type,int xsize,int ysize) {
vars_t *next;
vars_t *last;
void *o;
char *n;
int *resize;
char *arr[MAX_SIZE];
int arrval;
varsplit split;
functions *funcnext;
int statementcount;

/* Check if variable name is a reserved name */

statementcount=0;
 
do {
 if(statements[statementcount].statement == NULL) break;

 if(strcmpi(statements[statementcount].statement,name) == 0) return(BAD_VARNAME);
 
 statementcount++;

} while(statements[statementcount].statement != NULL);

statementcount=0;

/* Add entry to variable list */

if(currentfunction->vars == NULL) {			/* first entry */
  currentfunction->vars=malloc(sizeof(vars_t));		/* add new item to list */
  if(currentfunction->vars == NULL) return(NO_MEM);	/* can't resize */

  next=currentfunction->vars;
 }
 else
 {
  next=currentfunction->vars;						/* point to variables */

  while(next != NULL) {
   last=next;

   if(strcmpi(next->varname,name) == 0) return(VARIABLE_EXISTS);		/* variable exists */
   next=next->next;
  }

  last->next=malloc(sizeof(vars_t));		/* add new item to list */
  if(last->next == NULL) return(NO_MEM);	/* can't resize */

  next=last->next;
 }

/* add to end */

    switch(type) {
     case VAR_NUMBER:				/* double precision */			
	next->val=malloc((xsize*sizeof(double))*(ysize*sizeof(double)));

 	if(next->val == NULL)  return(NO_MEM);
        break;

     case VAR_STRING:				/* string */
	next->val=malloc(((xsize*MAX_SIZE)*(ysize*MAX_SIZE))+MAX_SIZE);
 	if(next->val == NULL)  return(NO_MEM);
       break;

     case VAR_INTEGER:	 			/* integer */
	next->val=malloc((xsize*sizeof(int))*(ysize*sizeof(int)));
 	if(next->val == NULL)  return(NO_MEM);
       break;

     case VAR_SINGLE:				/* single */	     
	next->val=malloc((xsize*sizeof(float))*(ysize*sizeof(float)));
 	if(next->val == NULL)  return(NO_MEM);
       break;
    }

 next->xsize=xsize;				/* set size */
 next->ysize=ysize;
 next->type=type;

 strcpy(next->varname,name);		/* set name */
 next->next=NULL;

 return(0);
}

/*
 * Update variable
 *
 * In: char *name	Variable name
       varval *val	Variable value
       int x		X subscript
       int y		Y subscript
 *
 * Returns -1 on error or 0 on success
 *
 */	
int UpdateVariable(char *name,varval *val,int x,int y) {
vars_t *next;
char *o;
varval *varv;

/* Find variable */

next=currentfunction->vars;

 while(next != NULL) {
   if(strcmpi(next->varname,name) == 0) {		/* already defined */

    if((x*y) > (next->xsize*next->ysize)) {		/* outside array */
	PrintError(BAD_ARRAY);
	return;
    }

/* update variable */

    switch(next->type) {
     case VAR_NUMBER:				/* double precision */			
       next->val[(y*next->ysize)+(x*next->xsize)].d=val->d;
       break;

     case VAR_STRING:				/* string */
       printf("x y=%s %d %d %d %d\n",next->varname,x,y,next->xsize,next->ysize);

       strcpy(next->val[(y*next->ysize)+(next->xsize*x)].s,val->s);
       break;

     case VAR_INTEGER:	 			/* integer */
       next->val[(y*next->ysize)+(x*next->xsize)].i=val->i;
       break;

     case VAR_SINGLE:				/* single */	     
       next->val[x*sizeof(float)+(y*sizeof(float))].f=val->f;
       break;
    }
 
    return(0);
   }

  next=next->next;
 }

 return(-1);
}

/*
 * Resize array
 *
 * In: char *name	Variable name
       int x		X subscript
       int y		Y subscript
 *
 * Returns -1 on error or 0 on success
 *
 */								
int ResizeArray(char *name,int x,int y) {
vars_t *next;
char *o;

int statementcount;
 
/* Find variable */

next=currentfunction->vars;

 while(next != NULL) {

   if(strcmpi(next->varname,name) == 0) {		/* found variable */
    if(realloc(next->val,(x*y)*sizeof(varval)) == NULL) return(-1);	/* resize buffer */
   
    next->xsize=x;		/* update x subscript */
    next->ysize=y;		/* update y subscript */
    return(0);
  }

  next=next->next;
 }

 return(-1);
}

/*
 * Get variable value
 *
 * In: char *name	Variable name
       varname *val	Variable value
 *
 * Returns -1 on error or 0 on success
 *
 */
int GetVariableValue(char *name,int x,int y,varval *val) {
vars_t *next;
varsplit split;
char *token;
char c;
char *subscriptptr;
int intval;
double floatval;

if(name == NULL) return(-1);

c=*name;

if(c >= '0' && c <= '9') {
   switch(GetVariableType(name)) {
      case VAR_NUMBER:				/* Double precision */
        val->d=atof(name);
	break;

       case VAR_INTEGER:			/* Integer */
        val->i=atoi(name);
	break;

       case VAR_SINGLE:				/* Single precision */
        val->f=atof(name);
	break;

       default:					/* Default type */
        val->d=atof(name);
	break;
    }

 return(0);
}

if(c == '"') {
 strcpy(val->s,name);
 return(0);
}

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {
   if(strcmpi(next->varname,name) == 0) {
    if((x*y) > (next->xsize*next->ysize)) {		/* outside array */
	PrintError(BAD_ARRAY);
	return;
    }

   switch(next->type) {
      case VAR_NUMBER:				/* Double precision */
        val->d=next->val[(y*next->ysize)+(x*next->xsize)].d;
        return(0);

       case VAR_STRING:			 	/* string */
        strcpy(val->s,next->val[(y*next->ysize)+(next->xsize*x)].s);
        return(0);

       case VAR_INTEGER:			/* Integer */
        val->i=next->val[(y*next->ysize)+(x*next->xsize)].i;
	return(0);

       case VAR_SINGLE:				/* Single precision */
        next->val[(y*next->ysize)+(x*next->xsize)].f=val->f;
	return(0);

       default:					/* Default type */
        val->d=next->val[(y*next->ysize)+(x*next->xsize)].d;
        return(0);
    }

  }

  next=next->next;
 }

return(-1);
}

/*
 * Get variable type
 *
 * In: char *name	Variable name
 *
 * Returns -1 on error or variable type on success
 *
 */

int GetVariableType(char *name) {
vars_t *next;
varsplit split;

if(name == NULL) return(-1);				/* Variable does not exist */

if(*name >= '0' && *name <= '9') return(VAR_NUMBER);	/* Literal number */

if(*name == '"') return(VAR_STRING);			/* Literal string */
 
/* Find variable name */

next=currentfunction->vars;

while(next != NULL) {  
  
 if(strcmpi(next->varname,name) == 0) {		/* Found variable */
  return(next->type);
 }

 next=next->next;
}

return(-1);
}

/*
 * Split variable into name and subscripts
 *
 * In: char *name	Variable name
       varsplit *split	Variable split object
 *
 * Returns -1 on error or 0 on success
 *
 */
int ParseVariableName(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end,varsplit *split) {

strcpy(split->name,tokens[start]);		/* copy name */

if(start == end) { /* no subscripts */
 split->x=0;
 split->y=0;
 return;
}


split->y=0;

if((strcmp(tokens[start+1],"(") == 0) || (strcmp(tokens[start+1],"[") == 0)) {

 if(strcmp(tokens[start+1],"(") == 0) split->arraytype=ARRAY_SUBSCRIPT;
 if(strcmp(tokens[start+1],"[") == 0) split->arraytype=ARRAY_SLICE;

 split->x=atoi(tokens[start+2]);		/* get x subscript */

 if(strcmp(tokens[start+3],",") == 0) split->y=atoi(tokens[start+4]); /* 2d array */
}
else
{
 split->x=atoi(tokens[start+1]);		/* get x subscript */

 if(strcmp(tokens[start+2],",") == 0) split->y=atoi(tokens[start+3]); /* 2d array */
}

return;
}


/*
 * Remove variable
 *
 * In: char *name	Variable name
 *
 * Returns -1 on error or 0 on success
 *
 */
int RemoveVariable(char *name) {
 vars_t *next;
 vars_t *last;

 next=currentfunction->vars;						/* point to variables */
 
 while(next != NULL) {
   last=next;
  
   if(strcmpi(next->varname,name) == 0) {			/* found variable */
     last->next=next->next;				/* point over link */

//    free(next);
    return;    
   }

  next=next->next;
 }
}

/*
 * Declare function
 *
 * In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
       int funcargcount			number of tokens
 *
 * Returns -1 on error or 0 on success
 *
 */
int DeclareFunction(char *tokens[MAX_SIZE][MAX_SIZE],int funcargcount) {
 functions *next;
 functions *last;
 char *linebuf[MAX_SIZE];
 char *savepos;
 int count;
 int sc;
 vars_t *varptr;
 vars_t *paramsptr;
 vars_t *paramslast;
 int typecount=0;

 if((currentfunction->stat & FUNCTION_STATEMENT) == FUNCTION_STATEMENT) return(NESTED_FUNCTION);
/* Check if function already exists */

 if(funcs == NULL) {
  funcs=malloc(sizeof(functions));		/* add new item to list */
  next=funcs;
 }
 else
 {
  next=funcs;						/* point to variables */
 
  while(next != NULL) {
   last=next;

   if(strcmpi(next->name,tokens[0]) == 0) return(FUNCTION_IN_USE);	/* already defined */

   next=next->next;
  }

  last->next=malloc(sizeof(functions));		/* add new item to list */
  if(last->next == NULL) return(NO_MEM);		/* can't resize */

  next=last->next;
 }

 strcpy(next->name,tokens[0]);				/* copy name */
 next->funcargcount=funcargcount;

/* skip ( and go to end */

 for(count=2;count<next->funcargcount-1;count++) {

   if(strcmpi(tokens[count+1], "AS") == 0) {		/* type */
     /* check if declaring variable with type */
 	  typecount=0;

 	 while(vartypenames[typecount] != NULL) {
	   if(strcmpi(vartypenames[typecount],tokens[count+2]) == 0) break;	/* found type */
  
	   typecount++;
	 }

	 if(vartypenames[typecount] == NULL) {		/* invalid type */
	   PrintError(BAD_TYPE);
	   return(-1);
	 }
   }
   else
   {
	typecount=0;
  }

/* add parameter */

 if(next->parameters == NULL) {
  next->parameters=malloc(sizeof(vars_t));
  paramsptr=next->parameters;
 }
 else
 {
  paramsptr=next->parameters;

  while(paramsptr != NULL) {
   paramslast=paramsptr;

   paramsptr=paramsptr->next;
  }
 
  paramslast->next=malloc(sizeof(vars_t));		/* add to end */
  paramsptr=paramslast->next;
 }
	
/* add function parameters */
 strcpy(paramsptr->varname,tokens[count]);
 paramsptr->type=typecount;
 paramsptr->xsize=0;
 paramsptr->ysize=0;
 paramsptr->val=NULL;

 next->vars=NULL;
 next->funcstart=currentptr;

 if(strcmpi(tokens[count+1], "AS") == 0) count += 3;		/* skip as and type */

}

/* get function return type */

 typecount=0;


if(strcmpi(tokens[funcargcount-1], "AS") == 0) {		/* type */
 while(vartypenames[typecount] != NULL) {
   if(strcmpi(vartypenames[typecount],tokens[count+1]) == 0) break;	/* found type */

   typecount++;
 }


 if(vartypenames[typecount] == NULL)  typecount=0;
}

next->return_type=typecount;

/* find end of function */

 do {
  currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

  if(TokenizeLine(linebuf,tokens,TokenCharacters) == -1) {			/* tokenize line */
   PrintError(SYNTAX_ERROR);
   return(-1);
  }

  if(strcmpi(tokens[0],"ENDFUNCTION") == 0) return;  
 
}    while(*currentptr != 0); 			/* until end */

PrintError(FUNCTION_NO_ENDFUNCTION);
return;
}

int CheckFunctionExists(char *name) {
functions *next;

next=funcs;						/* point to variables */

/* find function name */

while(next != NULL) {
 if(strcmpi(next->name,name) == 0) return(0);		/* found name */   

 next=next->next;
}

return(-1);
}


/*
 * Call function
 *
 * In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
       int funcargcount			number of tokens
 *
 * Returns -1 on error or 0 on success
 *
 */

double CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end) {
functions *next;
int count,countx;
int tc;
char *argbuf[MAX_SIZE][MAX_SIZE];
char *buf[MAX_SIZE];
varsplit split;
vars_t *vars;
varval val;
vars_t *parameters;
int varstc;

next=funcs;						/* point to variables */

/* find function name */

while(next != NULL) {
 if(strcmpi(next->name,tokens[start]) == 0) break;		/* found name */   

 next=next->next;
}

if(next == NULL) return(INVALID_STATEMENT);

next->vars=NULL;		/* no vars to begin with */

SubstituteVariables(start,end,tokens,tokens);			/* substitute variables */

callstack[callpos].callptr=currentptr;	/* save information aboutn the calling function */
callstack[callpos].funcptr=currentfunction;
callpos++;

callstack[callpos].callptr=next->funcstart;	/* information about the current function */
callstack[callpos].funcptr=next;

currentfunction=next;
currentptr=next->funcstart;

currentfunction->stat |= FUNCTION_STATEMENT;


/* add variables from parameters */

count=start+1;		/* skip function name and ( */

parameters=next->parameters;

while(parameters != NULL) {

  ParseVariableName(tokens,start,end,&split);

  CreateVariable(parameters->varname,parameters->type,split.x,split.y);
 
  switch(parameters->type) {
    case VAR_NUMBER:				/* number */
	val.d=atof(tokens[count]);
	break;

    case VAR_STRING:				/* string */
	strcpy(val.s,tokens[count]);
	break;

    case VAR_INTEGER:				/* integer */
	val.i=atoi(tokens[count]);
	break;

    case VAR_SINGLE:				/* single */
	val.f=atof(tokens[count]);
	break;
  }

   UpdateVariable(parameters->varname,&val,split.x,split.y);

   parameters=parameters->next;   
   count += 2;		/* skip , */
}

/* do function */

while(*currentptr != 0) {	
 currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

 tc=TokenizeLine(buf,argbuf,TokenCharacters);			/* tokenize line */
 if(tc == -1) {
  PrintError(SYNTAX_ERROR);
  return(-1);
 }

 if(strcmpi(argbuf[0],"ENDFUNCTION") == 0) break;

 ExecuteLine(buf);

 if(strcmpi(argbuf[0],"RETURN") == 0) break;

}

currentfunction->stat &= FUNCTION_STATEMENT;

count=0;

/* remove variables */


vars=currentfunction->vars;

while(vars != NULL) {
  RemoveVariable(vars->varname);
  vars=vars->next;
}

ReturnFromFunction();			/* return */

return;
}

int ReturnFromFunction(void) {

if(callpos-1 >= 0) {
 callpos--;
 currentfunction=callstack[callpos].funcptr;  
 currentptr=callstack[callpos].callptr;	/* restore information about the calling function */
 }
}

/*
 * Convert char * to int using base
 *
 * In: char *hex		char representation of number
       int base			Base
 *
 * Returns -1 on error or 0 on success
 *
 */
int atoi_base(char *hex,int base) {
int num=0;
char *b;
char c;
int count=0;
int shiftamount=0;

if(base == 10) shiftamount=1;	/* for decimal */

b=hex;
count=strlen(hex);		/* point to end */

b=b+(count-1);

while(count > 0) {
 c=*b;
 
 if(base == 16) {
  if(c >= 'A' && c <= 'F') num += (((int) c-'A')+10) << shiftamount;
  if(c >= 'a' && c <= 'f') num += (((int) c-'a')+10) << shiftamount;
  if(c >= '0' && c <= '9') num += ((int) c-'0') << shiftamount;

  shiftamount += 4;
  count--;
 }

 if(base == 8) {
  if(c >= '0' && c <= '7') num += ((int) c-'0') << shiftamount;

  shiftamount += 3;
  count--;
 }

 if(base == 2) {
  if(c >= '0' && c <= '1') num += ((int) c-'0') << shiftamount;

  shiftamount += 1;
  count--;
 }

 if(base == 10) {
  num += (((int) c-'0')*shiftamount);

  shiftamount =shiftamount*10;
  count--;
 }

 b--;
}

return(num);
}

/*
 * Substitute variable names with values
 *
 * In: int start		Start of variables in tokens array
       int end			End of variables in tokens array
       char *tokens[][MAX_SIZE] Tokens array
 *
 * Returns -1 on error or 0 on success
 *
 */

int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE],char *out[][MAX_SIZE]) {
int count;
varsplit split;
char *valptr;
char *buf[MAX_SIZE];
functions *next;
functions *func;
char *b;
char *d;
int countx;
int outcount;
varval val;
int tokentype;
int s;
int countz;
char *temp[MAX_SIZE][MAX_SIZE];


/* replace non-decimal numbers with decimal equivalents */
 for(count=start;count<end;count++) {
   if(memcmp(tokens[count],"0x",2) == 0 ) {	/* hex number */  
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,16),buf);
    strcpy(tokens[count],buf);
   }

   if(memcmp(tokens[count],"&",1) == 0) {				/* octal number */
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,8),buf);
    strcpy(tokens[count],buf);
   }

   if(memcmp(tokens[count],"0b",2) == 0) {					/* binary number */
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,2),tokens[count]);
   }
 }

outcount=start;

for(count=start;count<end;count++) { 
 tokentype=0;

 if(CheckFunctionExists(tokens[count]) == 0) {	/* user function */
	  tokentype=SUBST_FUNCTION;

	  s=count;	/* save start */
 	  count++;		/* skip ( */

	  for(countx=count;countx<end;countx++) {		/* find end of function call */
		if(strcmp(tokens[countx],")") == 0) break;
	  }
 

	  CallFunction(tokens,s,countx-1);

	  if(retval.type == VAR_STRING) {		/* returning string */   
	   sprintf(temp[outcount++],"\"%s\"",retval.s);
	  }
	  else if(retval.type == VAR_INTEGER) {		/* returning integer */
		 sprintf(temp[outcount++],"%d",retval.i);
  	  }
	  else if(retval.type == VAR_NUMBER) {		/* returning double */
		 sprintf(temp[outcount++],"%.6g",retval.d);		 		
	  }
	  else if(retval.type == VAR_SINGLE) {		/* returning single */
		 sprintf(temp[outcount++],"%f",retval.f);
	  }

	  count=countx+1;
	  continue;
  }

 if(IsVariable(tokens[count]) == 0) {
   for(countx=count;countx<end;countx++) {		/* find end of function call */    
    if(strcmp(tokens[countx],")") == 0) break;
   }

    ParseVariableName(tokens,count,countx-1,&split);

    tokentype=SUBST_VAR;
   
    GetVariableValue(split.name,split.x,split.y,&val);

    switch(GetVariableType(split.name)) {
	case VAR_STRING:

	   if(split.arraytype == ARRAY_SLICE) {		/* part of string */
		GetVariableValue(split.name,0,0,&val);

		b=&val.s;			/* get start */
		b += split.x;

		memset(temp[outcount],0,MAX_SIZE);
		d=temp[outcount++];
		*d++='"';

		if(split.y == 0) split.y=1;

		for(count=0;count < split.y;count++) {
		 *d++=*b++;
		}

		break;
	    }
	    else
	    {
	      GetVariableValue(split.name,split.x,split.y,&val);
	
	      printf("subst %s %d %d=%s\n",split.name,split.x,split.y,val.s);
              printf("var type=%d\n",GetVariableType(split.name));

	      strcpy(temp[outcount++],val.s);
            }

	    break;
	
	case VAR_NUMBER:	
	    sprintf(temp[outcount++],"%.6g",val.d);          
   	    break;

	case VAR_INTEGER:	
	    sprintf(temp[outcount++],"%d",val.i);
	    break;

       case VAR_SINGLE:	     
	    sprintf(temp[outcount++],"%f",val.f);
            break;

       default:
	    sprintf(temp[outcount++],"%.6g",val.d);          
   	    break;
       }
	
	while((count < end) && strcmp(tokens[count],")") != 0) count++;
	continue;
 }

 if(tokentype == 0) {		/* is not variable or function */
  strcpy(temp[outcount++],tokens[count]);  
 }   
}

/* copy tokens */

 memset(out,0,MAX_SIZE*MAX_SIZE);

 for(count=0;count<outcount;count++) {
  strcpy(out[count],temp[count]);
 }

}

/*
 * Conatecate strings
 *
 * In: int start		Start of variables in tokens array
       int end			End of variables in tokens array
       char *tokens[][MAX_SIZE] Tokens array
       varval *val 		Variable value to return conatecated strings
 *
 * Returns -1 on error or 0 on success
 *
 */
int ConatecateStrings(int start,int end,char *tokens[][MAX_SIZE],varval *val) {
int count;
char *b;
char *d;

SubstituteVariables(start,end,tokens,tokens);

val->type=VAR_STRING;

d=val->s;				/* copy token */

b=tokens[start];			/* point to first token */
if(*b == '"') {
// *d++='"';
 b++;			/* skip over quote */
}

while(*b != 0) {
 if(*b == '"') break;
 if(*b == 0) break;

 *d++=*b++;
}

for(count=start+1;count<end;count++) {

 if(strcmpi(tokens[count],"+") == 0) { 

    b=tokens[count+1];
    if(*b == '"') b++;			/* skip over quote */

    while(*b != 0) {			/* copy token */
     if(*b == '"') break;
     if(*b == 0) break;

     *d++=*b++;
    }

    count++;
   }
  }

*d++=0;
//*d++='"';
 return(0);
}

/*
 * Check variable type
 *
 * In: char *typename		Variable type as string
 *
 * Returns -1 on error or variable type on success
 *
 */
int CheckVariableType(char *typename) {
 int typecount=0;

 while(vartypenames[typecount] != NULL) {
   if(strcmpi(vartypenames[typecount],typename) == 0) break;	/* found type */
  
   typecount++;
 }

 if(vartypenames[typecount] == NULL) return(-1);		/* invalid type */

 return(typecount);
}

/*
 * Check if is variable
 *
 * In: varname		Variable name
 *
 * Returns -1 on error or 0 on success
 *
 */
int IsVariable(char *varname) {
vars_t *next;

next=currentfunction->vars;						/* point to variables */

while(next != NULL) {
 if(strcmpi(next->varname,varname) == 0) return(0);		/* variable exists */
 next=next->next;
}

return(-1);
}

