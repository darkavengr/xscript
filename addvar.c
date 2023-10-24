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

functions *funcs=NULL;
FUNCTIONCALLSTACK *currentfunction=NULL;
UserDefinedType *udt=NULL;

char *vartypenames[] = { "DOUBLE","STRING","INTEGER","SINGLE",NULL };
functionreturnvalue retval;

extern char *currentptr;
extern statement statements[];
vars_t *findvar;

int callpos=0;

/*
 * Initalize function call stack;
 *
 * In: Nothing
 *
 * Returns: -1 On error or 0 on success
 *
 */

extern char *TokenCharacters;

int InitializeFunctions(void) {
FUNCTIONCALLSTACK newfunc;

memset(&newfunc,0,sizeof(FUNCTIONCALLSTACK));
strcpy(newfunc.name,"main");

PushFunctionCallInformation(&newfunc);

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
 * In: name	Variable name
       type	Variable type
       xsize	Size of X subscript
       ysize	Size of Y subscript
 *
 * Returns -1 on error or 0 on success
 *
 */

int CreateVariable(char *name,char *type,int xsize,int ysize) {
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
UserDefinedType *usertype;
UserDefinedType *saveusertype;
UserDefinedTypeField *fieldptr;
int count;
UserDefinedTypeField *udtptr;

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

if(strcmpi(type,"DOUBLE") == 0) {		/* double precision */			
	next->val=malloc((xsize*sizeof(double))*(ysize*sizeof(double)));
 	if(next->val == NULL) return(NO_MEM);

	next->type_int=VAR_NUMBER;
}
else if(strcmpi(type,"STRING") == 0) {	/* string */			
	next->val=calloc(((xsize*MAX_SIZE)*(ysize*MAX_SIZE)),sizeof(char *));
 	if(next->val == NULL) return(NO_MEM);

	next->type_int=VAR_STRING;
}
else if(strcmpi(type,"INTEGER") == 0) {	/* integer */
	next->val=malloc((xsize*sizeof(int))*(ysize*sizeof(int)));
 	if(next->val == NULL) return(NO_MEM);

	next->type_int=VAR_INTEGER;
}
else if(strcmpi(type,"SINGLE") == 0) {	/* single */
	next->val=malloc((xsize*sizeof(float))*(ysize*sizeof(float)));
 	if(next->val == NULL) return(NO_MEM);

	next->type_int=VAR_SINGLE;
}
else {					/* user-defined type */	 
	next->type_int=VAR_UDT;

	usertype=GetUDT(type);
	if(usertype == NULL) return(BAD_TYPE);

	next->udt=malloc((xsize*sizeof(UserDefinedType))*(ysize*sizeof(UserDefinedType)));	/* allocate user-defined type */

	next->udt->field=malloc(sizeof(UserDefinedTypeField));		/* allocate field in user-defined type */
	if(next->udt->field == NULL) return(NO_MEM);

	udtptr=next->udt->field;

	/* copy udt from type definition to variable */

	for(count=0;count<(xsize*ysize);count++) {
		fieldptr=usertype->field;		/* point to field */

		while(fieldptr != NULL) {				
			/* copy field entries */
			strcpy(udtptr->fieldname,fieldptr->fieldname);
			strcpy(udtptr->type,fieldptr->type);

			udtptr->fieldval=malloc(udtptr->xsize*udtptr->ysize);
			if(udtptr->fieldval == NULL) return(NO_MEM);
		
			udtptr->next=malloc(sizeof(UserDefinedTypeField));		/* allocate field in user-defined type */
			if(udtptr->next == NULL) return(NO_MEM);

			fieldptr=fieldptr->next;
		}			
	}
}

next->xsize=xsize;				/* set size */
next->ysize=ysize;

strcpy(next->type,type);		/* set type */
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
int UpdateVariable(char *name,char *fieldname,varval *val,int x,int y) {
vars_t *next;
char *o;
varval *varv;
char *strptr;
varsplit split;
UserDefinedType *udtptr;
UserDefinedTypeField *fieldptr;

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) break;		/* already defined */
  
 	next=next->next;
}

if(next == NULL) return(VARIABLE_DOES_NOT_EXIST);
if((x*y) > (next->xsize*next->ysize)) return(BAD_ARRAY);		/* outside array */

/* update variable */

if(strcmpi(next->type,"DOUBLE") == 0) {		/* double precision */			
       next->val[(y*next->ysize)+(x*next->xsize)].d=val->d;
       return(0);

}
else if(strcmpi(next->type,"STRING") == 0) {	/* string */			
       if(next->val[( (y*next->ysize)+(next->xsize*x))].s == NULL) {		/* if string element not allocated */

	 next->val[((y*next->ysize)+(next->xsize*x))].s=malloc(strlen(val->s));	/* allocate memory */
     	 strcpy(next->val[((y*next->ysize)+(next->xsize*x))].s,val->s);		/* assign value */
       } 

       if( strlen(val->s) > strlen(next->val[( (y*next->ysize)+(next->xsize*x))].s)) {	/* if string element larger */
	 realloc(next->val[((y*next->ysize)+(next->xsize*x))].s,strlen(val->s));	/* resize memory */

     	 strcpy(next->val[((y*next->ysize)+(next->xsize*x))].s,val->s);		/* assign value */
       }

       return(0);
}
else if(strcmpi(next->type,"INTEGER") == 0) {	/* integer */
       next->val[(y*next->ysize)+(x*next->xsize)].i=val->i;
       return(0);
}
else if(strcmpi(next->type,"SINGLE") == 0) {	/* single */
       next->val[x*sizeof(float)+(y*sizeof(float))].f=val->f;
       return(0);
}
else {					/* user-defined type */	
	fieldptr=next->udt->field;	/* point to fields in udt */

	while(fieldptr != NULL) {		/* search fields */
	 
	  if(strcmpi(fieldptr->fieldname) == 0) {	/* found name */
		if(fieldptr->type == VAR_INTEGER) {
			fieldptr->fieldval->i=val->i;
		        return(0);
		}
		else if(fieldptr->type == VAR_NUMBER) {
			fieldptr->fieldval->d=val->d;
		        return(0);
		}
		else if(fieldptr->type == VAR_SINGLE) {
			fieldptr->fieldval->i=val->f;
		        return(0);
		}	
		else if(fieldptr->type == VAR_STRING) {
			if(next->val[( (y*next->ysize)+(next->xsize*x))].s == NULL) {		/* if string element not allocated */
			 next->val[((y*next->ysize)+(next->xsize*x))].s=malloc(strlen(val->s));	/* allocate memory */
		     	 strcpy(next->val[((y*next->ysize)+(next->xsize*x))].s,val->s);		/* assign value */
		        } 

		        if( strlen(val->s) > strlen(next->val[( (y*next->ysize)+(next->xsize*x))].s)) {	/* if string element larger */
			 realloc(next->val[((y*next->ysize)+(next->xsize*x))].s,strlen(val->s));	/* resize memory */	
		     	 strcpy(next->val[((y*next->ysize)+(next->xsize*x))].s,val->s);		/* assign value */
       			}

		        return(0);
		}
	     }

          fieldptr=fieldptr->next;
       }
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
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy) {
vars_t *next;
varsplit split;
UserDefinedTypeField *udtfield;
char c;

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

       default:
        val->d=atof(name);
	break;
    }

 return(0);
}

if(c == '"') {
 val->s=malloc(strlen(name));				/* allocate string */
 if(val->s == NULL) return(-1);

 strcpy(val->s,name);
 return(0);
}

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) break;		/* already defined */
  
 	next=next->next;
}

if(next == NULL) return(-1);

if((x*y) > (next->xsize*next->ysize)) return(BAD_ARRAY);		/* outside array */

if(strcmpi(next->type,"DOUBLE") == 0) {
        val->d=next->val[(y*next->ysize)+(x*next->xsize)].d;
        return(0);
}
else if(strcmpi(next->type,"STRING") == 0) {
	val->s=malloc(strlen(next->val[( (y*next->ysize)+(next->xsize*x))].s));				/* allocate string */
	if(val->s == NULL) return(-1);

        strcpy(val->s,next->val[( (y*next->ysize)+(next->xsize*x))].s);
        return(0);
}
else if(strcmpi(next->type,"INTEGER") == 0) {
        val->i=next->val[(y*next->ysize)+(x*next->xsize)].i;
	return(0);
}
else if(strcmpi(next->type,"SINGLE") == 0) {
        val->f=next->val[(y*next->ysize)+(x*next->xsize)].f;
	return(0);
}
else {					/* User-defined type */
udtfield=next->udt;

	while(udtfield != NULL) {
	 	if(strcmp(udtfield->fieldname,fieldname) == 0) {	/* found field */
			 if(strcmpi(next->type,"DOUBLE") == 0) {
			        val->d=udtfield->fieldval[(y*fieldy)+(x*fieldx)].d;
			        return(0);
			 }
			 else if(strcmpi(next->type,"STRING") == 0) {
				val->s=malloc(strlen(next->val[( (y*fieldy)+(x*fieldx))].s));				/* allocate string */
				if(val->s == NULL) return(NO_MEM);

        			strcpy(val->s,udtfield->fieldval[( (y*fieldy)+(x*fieldx))].s);
			        return(0);
			 }
			 else if(strcmpi(next->type,"INTEGER") == 0) {
			        val->i=udtfield->fieldval[(y*fieldy)+(x*fieldx)].i;
				return(0);
			 }
			 else if(strcmpi(next->type,"SINGLE") == 0) {
			        val->f=next->val[(y*fieldy)+(x*fieldx)].f;
				return(0);
			 }
			 else {
			  return(BAD_TYPE);
			 }	
		}

		udtfield=udtfield->next;
	}
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
 if(strcmpi(next->varname,name) == 0) return(next->type_int);		/* Found variable */

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
int exprparse=0;
int fieldstart=0;
int subscriptend=0;
int fieldsubscriptstart=0;

strcpy(split->name,tokens[start]);		/* copy name */

if(start == end) { /* no subscripts */
 split->x=0;
 split->y=0;
 return;
}


split->y=0;

/* If array or slice */

if((strcmp(tokens[start+1],"(") == 0) || (strcmp(tokens[start+1],"[") == 0)) {

 if(strcmp(tokens[start+1],"(") == 0) split->arraytype=ARRAY_SUBSCRIPT;
 if(strcmp(tokens[start+1],"[") == 0) split->arraytype=ARRAY_SLICE;
 
/* find end of array subscripts */

 for(subscriptend=start+1;subscriptend=end;subscriptend++) {
   if(strcmp(tokens[subscriptend],")") == 0) break;
 }

 for(exprparse=start+2;exprparse<end;exprparse++) {
    if(strcmp(tokens[exprparse],",") == 0) {		 /* 2d array */
     	 split->x=doexpr(tokens,start+2,exprparse);
	 split->y=doexpr(tokens,exprparse+1,subscriptend);
	 return;
    }
 }

 split->x=doexpr(tokens,start+2,end+1);
 split->y=0;
}
else
{
	split->x=0;
	split->y=1;
}

/* find start of field name */

for(fieldstart=subscriptend+1;fieldstart<end;fieldstart++) {
	 if(strcmp(tokens[fieldstart],".") == 0) {		 /* found start of field name */

  		if(fieldstart < end-2) return(SYNTAX_ERROR);

/* find field subscripts */

		  for(fieldsubscriptstart=fieldstart+1;fieldsubscriptstart<end;fieldsubscriptstart++) {
			if((strcmp(tokens[fieldsubscriptstart],"(") == 0) || (strcmp(tokens[fieldsubscriptstart],"[") == 0)) {

			  for(exprparse=start+2;exprparse<end;exprparse++) {
    				if(strcmp(tokens[exprparse],",") == 0) {		 /* 3d array */
				 split->x=doexpr(tokens,start+2,exprparse);
				 split->y=doexpr(tokens,exprparse+1,subscriptend);
				 return;
				}
			  }

			  if(exprparse == end) {			/* 3d array */  
		       	  	split->fieldx=doexpr(tokens,start+2,end+1);
			        split->fieldy=0;
			  }

			break;
		   }
	 	  }
	
	 }

}

  strcpy(split->fieldname,tokens[fieldsubscriptstart+1]);	/* copy field name */
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
    return(0);    
   }

  next=next->next;
 }

 return(-1);
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
 char *vartype[MAX_SIZE];
 UserDefinedType *udtptr;
 UserDefinedTypeField *udtfieldptr;

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
	   if(strcmpi(vartypenames[typecount],tokens[count+2]) == 0) { 		/* found type */
		 strcpy(vartype,vartypenames[typecount]);
		 break;
  	   }

	   typecount++;
	 }

	 if(vartypenames[typecount] == NULL) {		/* user-defined type */
	//	udtptr=GetUDT(tokens[count+2]);
	//	if(udtptr == NULL) return(BAD_TYPE);

	//	strcpy(vartype,tokens[count+2]);
	 }
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

  if(vartypenames[typecount] != NULL) {
	 strcpy(paramsptr->type,vartypenames[typecount]);
  }
  else
  {
 	strcpy(paramsptr->type,tokens[count+2]);
  }

  paramsptr->xsize=0;
  paramsptr->ysize=0;
  paramsptr->val=NULL;

  next->funcstart=currentptr;

  if(strcmpi(tokens[count+1], ")") == 0) break;		/* at end */

  if(strcmpi(tokens[count+1], "AS") == 0) count += 3;		/* skip as and type */
 }


/* get function return type */

 typecount=0;


 if(strcmpi(tokens[funcargcount-1], "AS") == 0) {		/* type */
  while(vartypenames[typecount] != NULL) {
    if(strcmpi(vartypenames[typecount],tokens[count+1]) == 0) break;	/* found type */

    typecount++;
  }
 }

 if(vartypenames[typecount] == NULL) {		/* user-defined type */
  udtptr=GetUDT(tokens[count+1]);
  if(udtptr == NULL) return(BAD_TYPE);
 }

 strcpy(next->returntype,tokens[funcargcount-1]);

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
int endcount;
FUNCTIONCALLSTACK newfunc;
UserDefinedType userdefinedtype;

next=funcs;						/* point to variables */
/* find function name */

while(next != NULL) {
 if(strcmpi(next->name,tokens[start]) == 0) break;		/* found name */   

 next=next->next;
}

if(next == NULL) return(INVALID_STATEMENT);

SubstituteVariables(start+2,end,tokens,tokens);			/* substitute variables */

/* save information about the calling function. The calling function is already on the stack */

currentfunction->callptr=currentptr;

/* save information about the called function */
memset(&newfunc,0,sizeof(FUNCTIONCALLSTACK));

strcpy(newfunc.name,next->name);

newfunc.callptr=next->funcstart;
currentptr=next->funcstart;
newfunc.lc=next->lc;
newfunc.stat |= FUNCTION_STATEMENT;

strcpy(newfunc.returntype,next->returntype);

PushFunctionCallInformation(&newfunc);

/* add variables from parameters */

parameters=next->parameters;
count=start+2;		/* skip function name and ( */

while(parameters != NULL) {

  CreateVariable(parameters->varname,parameters->type,split.x,split.y);
 
  if(strcmpi(parameters->type,"DOUBLE") == 0) {
	val.d=atof(tokens[count]);
  }
  else if(strcmpi(parameters->type,"STRING") == 0) {
	strcpy(val.s,tokens[count]);
  }
  else if(strcmpi(parameters->type,"INTEGER") == 0) {
	val.i=atoi(tokens[count]);
  }
  else if(strcmpi(parameters->type,"SINGLE") == 0) {
	val.f=atof(tokens[count]);
  }
  else {
   	
  }

   UpdateVariable(parameters->varname,NULL,&val,split.x,split.y);

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
PopFunctionCallInformation();
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
varval subst_returnvalue;

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
		if(strcmp(tokens[countx],")") == 0) {
			countx++;
			break;
		}
	  }
 

	  CallFunction(tokens,s,countx-1);

	  get_return_value(&subst_returnvalue);

	  if(retval.val.type == VAR_STRING) {		/* returning string */   
	   sprintf(temp[outcount++],"\"%s\"",retval.val.s);
	  }
	  else if(retval.val.type == VAR_INTEGER) {		/* returning integer */
		 sprintf(temp[outcount++],"%d",retval.val.i);
  	  }
	  else if(retval.val.type == VAR_NUMBER) {		/* returning double */
		 sprintf(temp[outcount++],"%.6g",retval.val.d);		 		
	  }
	  else if(retval.val.type == VAR_SINGLE) {		/* returning single */
		 sprintf(temp[outcount++],"%f",retval.val.f);
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
   
    GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy);

    switch(GetVariableType(split.name)) {
	case VAR_STRING:
	   if(split.arraytype == ARRAY_SLICE) {		/* part of string */
		GetVariableValue(split.name,split.fieldname,0,0,&val,split.fieldx,split.fieldy);

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
	      if(*tokens[count] == '"') {
		strcpy(temp[outcount++],tokens[count]);
	      }
	      else
	      {
	        GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy);
	  	sprintf(temp[outcount++],"\"%s\"",val.s);
	      }
	      
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
	
	countx=count;

	while(countx < end) {
	  if(strcmp(tokens[countx],")") != 0) break;
          
          countx++;
        }

        if(countx > count) count=countx;

	continue;
    }

 if(tokentype == 0) {		/* is not variable or function */
  strcpy(temp[outcount++],tokens[count]);  
 }   
}

/* copy tokens */

 memset(out,0,MAX_SIZE*MAX_SIZE);

 for(count=0;count<outcount;count++) {
//  printf("subst=%s\n",temp[count]);

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

//SubstituteVariables(start,end,tokens,tokens);

val->type=VAR_STRING;
val->s=malloc(MAX_SIZE);		/* initial size */
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

    printf("%s %d\n",tokens[count+1],GetVariableType(tokens[count+1]));

    if(GetVariableType(tokens[count+1]) != VAR_STRING) {	/* not a string literal or string variable */
     PrintError(TYPE_ERROR);
     return(TYPE_ERROR);
    }

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

int PushFunctionCallInformation(FUNCTIONCALLSTACK *func) {
FUNCTIONCALLSTACK *next;
FUNCTIONCALLSTACK *previous;

if(currentfunction == NULL) {
 currentfunction=malloc(sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */

 if(currentfunction == NULL) {
  PrintError(NO_MEM);
  return(-1);
 }

 currentfunction->last=NULL;
 next=currentfunction;
}
else
{
 currentfunction->next=malloc(sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */
 if(currentfunction->next == NULL) {
  PrintError(NO_MEM);
  return(-1);
 }

next=currentfunction->next;
}

memset(next,0,sizeof(FUNCTIONCALLSTACK));
memcpy(next,func,sizeof(FUNCTIONCALLSTACK));

previous=currentfunction;
currentfunction=next;
next->last=previous;
next->next=NULL;
}

int PopFunctionCallInformation(void) {
FUNCTIONCALLSTACK *thisfunction;

thisfunction=currentfunction;

currentfunction=currentfunction->last;

currentptr=currentfunction->callptr;
//free(thisfunction);
}

int FindFirstVariable(vars_t *var) {
if(currentfunction->vars == NULL) return(-1);

findvar=currentfunction->vars;

memcpy(var,currentfunction->vars,sizeof(vars_t));
}

int FindNextVariable(vars_t *var) {
findvar=findvar->next;

if(findvar == NULL) return(-1);

memcpy(var,findvar,sizeof(vars_t));
}

int FindVariable(char *name,vars_t *var) {
vars_t *next;

next=currentfunction->vars;

while(next != NULL) {
 if(strcmp(next->varname,name) == 0) {
	 memcpy(var,next,sizeof(vars_t));
	 return(0);
 }

 next=next->next;
}

return(-1);
}

UserDefinedType *GetUDT(char *name) {
 UserDefinedType *next=udt;

 while(next != NULL) {
  if(strcmpi(next->name,name) == 0) return(next);	/* found UDT */

  next=next->next;
 }

 return(NULL);
}

int AddUserDefinedType(UserDefinedType *newudt) {
 UserDefinedType *next=udt;
 UserDefinedType *last;

 while(next != NULL) {
  last=next;

  if(strcmpi(next->name,newudt->name) == 0) return(-1);	/* udt exists */

  next=next->next;
 }

 last->next=malloc(sizeof(UserDefinedType));		/* add link to the end */
 if(last->next == NULL) return(NO_MEM); 

 memcpy(last->next,newudt,sizeof(UserDefinedType));
 return(0);
}

int CopyAllUDT(UserDefinedType *source,UserDefinedType *dest) {
UserDefinedTypeField *sourcenext;
UserDefinedTypeField *destnext;
int count;

dest->field=malloc(sizeof(UserDefinedTypeField));
if(dest->field == NULL) return(NO_MEM);

sourcenext=source->field;
destnext=dest->field;

while(sourcenext != NULL) {
/* copy rather than duplicate */

 strcpy(destnext->fieldname,sourcenext->fieldname);
 destnext->xsize=sourcenext->xsize;
 destnext->ysize=sourcenext->ysize;
 destnext->type=sourcenext->type;

 if(sourcenext->type == VAR_NUMBER) {		/* double precision */			
	destnext->fieldval=malloc((sourcenext->xsize*sizeof(double))*(sourcenext->ysize*sizeof(double)));
 	if(destnext->fieldval == NULL) return(NO_MEM);

	memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(double))*(sourcenext->ysize*sizeof(double)));
 }
 else if(sourcenext->type == VAR_SINGLE) {		/* double precision */			
	destnext->fieldval=malloc((sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
 	if(destnext->fieldval == NULL) return(NO_MEM);

	memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
 }
 if(sourcenext->type == VAR_INTEGER) {		/* integer */
	destnext->fieldval=malloc((sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
 	if(destnext->fieldval == NULL) return(NO_MEM);

	memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
 }
 else if(sourcenext->type == VAR_STRING) {	/* string */			
	destnext->fieldval=calloc(((sourcenext->xsize*MAX_SIZE)*(sourcenext->ysize*MAX_SIZE)),sizeof(char *));
 	if(destnext->fieldval == NULL) return(NO_MEM);

	/* copy strings in array */

	for(count=0;count<(sourcenext->xsize*sourcenext->ysize);count++) {
	 	if(sourcenext->fieldval->s[count] == NULL) {
			sourcenext->fieldval[count].s=malloc(strlen(destnext->fieldval[count].s));
			if(sourcenext->fieldval[count].s == NULL) return(NO_MEM);

			strcpy(destnext->fieldval[count].s,sourcenext->fieldval->s[count]);
		}
	}
  }

  destnext->next=malloc(sizeof(UserDefinedTypeField));
  if(dest->next == NULL) return(NO_MEM);
 }
}

vars_t *GetVariablePointer(char *name) {
vars_t *next;

next=currentfunction->vars;

while(next != NULL) {
 if(strcmpi(next->varname,name) == 0) return(next);	/* found variable */

 next=next->next;
}

return(NULL);
}

