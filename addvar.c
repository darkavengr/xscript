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

ParseVariableName(name,&split);				/* parse variable name */

/* Check if variable name is a reserved name */

statementcount=0;
 
do {
 if(statements[statementcount].statement == NULL) break;

 if(strcmpi(statements[statementcount].statement,split.name) == 0) return(BAD_VARNAME);
 
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

   if(strcmpi(next->varname,split.name) == 0) return(-1);		/* variable exists */
   next=next->next;
  }

  last->next=malloc(sizeof(vars_t));		/* add new item to list */
  if(last->next == NULL) return(NO_MEM);	/* can't resize */

  next=last->next;
 }

/* add to end */

 next->val=malloc(sizeof(varval)*(xsize*ysize));
 if(next->val == NULL) {
  free(next);
  return(NO_MEM);
 }
  
 next->xsize=xsize;				/* set size */
 next->ysize=ysize;
 next->type=type;

 strcpy(next->varname,split.name);		/* set name */
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
varsplit split;
varval *varv;

ParseVariableName(name,&split);

/* Find variable */

next=currentfunction->vars;

 while(next != NULL) {
   if(strcmpi(next->varname,split.name) == 0) {		/* already defined */

    if((x*y) > (next->xsize*next->ysize)) {		/* outside array */
	PrintError(BAD_ARRAY);
	return;
    }

/* update variable */

    switch(next->type) {
     case VAR_NUMBER:				/* double precision */			
       next->val[x*y].d=val->d;
       break;

     case VAR_STRING:				/* string */
       strcpy(next->val[x*y].s,val->s);
       break;

     case VAR_INTEGER:	 			/* integer */
       next->val[x*y].i=val->i;
       break;

     case VAR_SINGLE:				/* single */	     
       next->val[x*y].f=val->f;
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
varsplit split;
int statementcount;
 
ParseVariableName(name,&split);				/* parse variable name */

/* Find variable */

next=currentfunction->vars;

 while(next != NULL) {

   if(strcmpi(next->varname,split.name) == 0) {		/* found variable */
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
int GetVariableValue(char *name,varval *val) {
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
 val->d=atof(name);
 return(0);
}

if(c == '"') {
 strcpy(val->s,name);
 return(0);
}

ParseVariableName(name,&split);

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {

 if(strcmpi(next->varname,split.name) == 0) {

    if((split.x*split.y) > (next->xsize*next->ysize)) {		/* outside array */
	PrintError(BAD_ARRAY);
	return;
    }

   switch(next->type) {
      case VAR_NUMBER:				/* Double precision */
        val->d=next->val[split.x*split.y].d;
        return(0);

       case VAR_STRING:			 	/* string */
        strcpy(val->s,next->val[split.x*split.y].s);
        return(0);

       case VAR_INTEGER:			/* Integer */
        val->i=next->val[split.x*split.y].i;
	return(0);

       case VAR_SINGLE:				/* Single precision */
        next->val[split.x*split.y].f=val->f;
	return(0);

       default:					/* Default type */
        val->d=next->val[split.x*split.y].d;
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

ParseVariableName(name,&split);
 
/* Find variable name */

next=currentfunction->vars;

while(next != NULL) {  
  
 if(strcmpi(next->varname,split.name) == 0) {		/* Found variable */
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
int ParseVariableName(char *name,varsplit *split) {
char *arrpos;
char *arrx[MAX_SIZE];
char *arry[MAX_SIZE];
char *o;
char *commapos;
char *b;
char *tokens[10][MAX_SIZE];
int tc;

memset(arrx,0,MAX_SIZE);			/* clear buffer */
memset(arry,0,MAX_SIZE);			/* clear buffer */

memset(split,0,sizeof(varsplit)-1);

if((strpbrk(name,"(") == NULL) && (strpbrk(name,"[") == NULL)) {				/* find start of subscript */
 strcpy(split->name,name);
 return;
}

o=name;				/* copy name */
b=split->name;

while(*o != 0) {
 if(*o == '(') {
	split->arraytype=ARRAY_SUBSCRIPT;
	break;
 }

 if(*o == '[') {
	split->arraytype=ARRAY_SLICE;
	break;
 }

 *b++=*o++;
}

/* Get subscripts */

commapos=strpbrk(name,",");	/* find comma position */
if(commapos == NULL) {			/* 2d array */
 	o++;				/* copy subscript */
	 b=arrx;

	 while(*o != 0) {
  		if(*o == ']' || *o == ')') break;
		*b++=*o++;
		 }

 	o--;
 	//*o=0;	

         tc=TokenizeLine(arrx,tokens,"+-*/<>=!%~|&");			/* tokenize line */

	 split->x=doexpr(tokens,0,tc);		/* get x pos */
	 split->y=1;

	 return;
} 
else
{				/* 3d array */

	 o++;				/* copy subscript */
 	b=arrx;

	 while(*o != 0) {
	  if(*o == ',') break;
	  *b++=*o++;
	 }

	o++;
	 b=arry;

	 while(*o != 0) {
		  if((*o == ']') || (*o == ')')) break;
		  *b++=*o++;
	 }

         tc=TokenizeLine(arrx,tokens,"+-*/<>=!%~|&");			/* tokenize line */
	 split->x=doexpr(tokens,0,tc);		/* get x pos */

    	 tc=TokenizeLine(arry,tokens,"+-*/<>=!%~|&");			/* tokenize line */
	 split->y=doexpr(tokens,0,tc);		/* get x pos */
	 return;
 	}
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
 varsplit split;

 ParseVariableName(name,&split);				/* parse variable name */
 
 next=currentfunction->vars;						/* point to variables */
 
 while(next != NULL) {
   last=next;
  
   if(strcmpi(next->varname,split.name) == 0) {			/* found variable */
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
 * In: char *name		Function name
       char *args		Comma-separated function arguments
       int function_return_type	Function return type
 *
 * Returns -1 on error or 0 on success
 *
 */
int DeclareFunction(char *name,char *args,int function_return_type) {
 functions *next;
 functions *last;
 char *linebuf[MAX_SIZE];
 char *savepos;
 char *tokens[MAX_SIZE][MAX_SIZE];
 char *tokensplit[10][MAX_SIZE];
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

   if(strcmpi(next->name,name) == 0) return(FUNCTION_IN_USE);	/* already defined */

   next=next->next;
  }

  last->next=malloc(sizeof(functions));		/* add new item to list */
  if(last->next == NULL) return(NO_MEM);		/* can't resize */

  next=last->next;
 }

 strcpy(next->name,name);				/* copy name */

/* split each variable into name and type */
 next->funcargcount=TokenizeLine(args,tokens,",");			/* copy args */

 for(count=0;count<next->funcargcount;count++) {
  sc=TokenizeLine(tokens[count],tokensplit," ");			/* copy args */

   if(strcmpi(tokensplit[1], "AS") == 0) {		/* type */
     /* check if declaring variable with type */
 	  typecount=0;

 	 while(vartypenames[typecount] != NULL) {
	   if(strcmpi(vartypenames[typecount],tokensplit[2]) == 0) break;	/* found type */
  
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
 strcpy(paramsptr->varname,tokensplit[0]);
 paramsptr->type=typecount;
 paramsptr->xsize=0;
 paramsptr->ysize=0;
 paramsptr->val=NULL;

 next->vars=NULL;
 next->funcstart=currentptr;
 next->return_type=function_return_type;
}

/* find end of function */

 do {
  currentptr=ReadLineFromBuffer(currentptr,linebuf,LINE_SIZE);			/* get data */

  TokenizeLine(linebuf,tokens,"+-*/<>=!%~|&");			/* tokenize line */

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
 * In: char *name		Function name
       char *args		Comma-separated function arguments
 *
 * Returns -1 on error or 0 on success
 *
 */

double CallFunction(char *name,char *args) {
functions *next;
char *argbuf[10][MAX_SIZE];
int count;
int tc;
char *buf[MAX_SIZE];
varsplit split;
vars_t *vars;
varval val;
vars_t *parameters;
int varstc;

next=funcs;						/* point to variables */

/* find function name */

while(next != NULL) {
 if(strcmpi(next->name,name) == 0) break;		/* found name */   

 next=next->next;
}

if(next == NULL) return(INVALID_STATEMENT);

next->vars=NULL;		/* no vars to begin with */

varstc=TokenizeLine(args,argbuf,",");			/* tokenize line */

SubstituteVariables(0,varstc,argbuf);				/* substitute variables */

callstack[callpos].callptr=currentptr;	/* save information aboutn the calling function */
callstack[callpos].funcptr=currentfunction;
callpos++;

callstack[callpos].callptr=next->funcstart;	/* information about the current function */
callstack[callpos].funcptr=next;

currentfunction=next;
currentptr=next->funcstart;

currentfunction->stat |= FUNCTION_STATEMENT;


/* add variables from parameters */

parameters=next->parameters;
count=0;

while(parameters != NULL) {

  ParseVariableName(parameters->varname,&split);

  CreateVariable(parameters->varname,parameters->type,split.x,split.y);
 
  switch(parameters->type) {
    case VAR_NUMBER:				/* number */
	val.d=atof(argbuf[count]);
	break;

    case VAR_STRING:				/* string */
	strcpy(val.s,argbuf[count]);
	break;

    case VAR_INTEGER:				/* integer */
	val.i=atoi(argbuf[count]);
	break;

    case VAR_SINGLE:				/* single */
	val.f=atof(argbuf[count]);
	break;
  }

   UpdateVariable(parameters->varname,&val,split.x,split.y);

   parameters=parameters->next;   
   count++;
}

/* do function */

while(*currentptr != 0) {	
 currentptr=ReadLineFromBuffer(currentptr,buf,LINE_SIZE);			/* get data */

 tc=TokenizeLine(buf,argbuf,"+-*/<>=!%~|&");			/* tokenize line */

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

// printf("currentptr=%lX\n",currentptr);
// asm("int $3");

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

int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE]) {
int count;
varsplit split;
char *valptr;
char *buf[MAX_SIZE];
functions *next;
char *functionargs[MAX_SIZE];
functions *func;
char *b;
char *d;
int countx;
double ret;
varval val;
int foundfunc=FALSE;

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

/* replace variables with values */

for(count=start;count<end;count++) { 

  
  b=tokens[count];
  d=buf;

/* get function name */

 memset(buf,0,MAX_SIZE);

  while(*b != 0) {
   if(*b == '(') {		/* is function? */
	foundfunc=TRUE;
	break;
   }

   *d++=*b++;
  }

  if(foundfunc == TRUE) {
	  d=functionargs;
	  b++;
	
/* get function args */

	  while(*b != 0) {
	  	if(*b == ')') break;

   		*d++=*b++;
	  }

	  if(CallFunction(buf,functionargs) == -1) exit(-1);	/* error calling function */

	  if(retval.type == VAR_STRING) {		/* returning string */   
	   sprintf(tokens[count],"\"%s\"",retval.s);
	  }
	  else if(retval.type == VAR_INTEGER) {		/* returning integer */
		 sprintf(tokens[count],"%d",retval.i);
  	  }
	  else if(retval.type == VAR_NUMBER) {		/* returning double */
		 sprintf(tokens[count],"%.6g",retval.d);
	  }
	  else if(retval.type == VAR_SINGLE) {		/* returning single */
		 sprintf(tokens[count],"%f",retval.f);
	  }

	  continue;
  }

 ParseVariableName(tokens[count],&split);

 if(GetVariableValue(split.name,&val) != -1) {		/* is variable */

   switch(GetVariableType(tokens[count])) {
	case VAR_STRING:
	    if(split.arraytype == ARRAY_SLICE) {		/* part of string */
		b=&val.s;			/* get start */
		b += split.x;

		memset(tokens[count],0,MAX_SIZE);
		d=tokens[count];
		*d++='"';

		for(count=0;count < split.y+1;count++) {
		 *d++=*b++;
		}

		break;
	    }
	    else
	    {
	     d=tokens[count];
	     *d++='"';
		
	     strcpy(tokens[count],val.s);
            }

	    break;
	
	case VAR_NUMBER:		   
	    sprintf(tokens[count],"%.6g",val.d);          
   	    break;

	case VAR_INTEGER:	
	    sprintf(tokens[count],"%d",val.i);
	    break;

       case VAR_SINGLE:	     
	    sprintf(tokens[count],"%f",val.f);
            break;
	}
	
     }
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

SubstituteVariables(start,end,tokens);

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

