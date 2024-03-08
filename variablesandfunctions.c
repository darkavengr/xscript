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

/* Variables and functions */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "size.h"
#include "variablesandfunctions.h"
#include "errors.h"
#include "dofile.h"
#include "evaluate.h"

functions *funcs=NULL;
FUNCTIONCALLSTACK *currentfunction=NULL;
UserDefinedType *udt=NULL;

char *vartypenames[] = { "DOUBLE","STRING","INTEGER","SINGLE",NULL };
functionreturnvalue retval;
vars_t *findvar;

int callpos=0;

/*
 *  Initalize function call stack;
 * 
 *  In: Nothing
 * 
 *  Returns: -1 on error or 0 on success
 * 
  */

extern char *TokenCharacters;

int InitializeFunctions(void) {
FUNCTIONCALLSTACK newfunc;

/* create main function */

memset(&newfunc,0,sizeof(FUNCTIONCALLSTACK));
strcpy(newfunc.name,"main");

PushFunctionCallInformation(&newfunc);

return(0);
}

/*
 *  Free function list
 * 
 *  In: Nothing
 * 
 *  Returns: Nothing
 * 
  */

void FreeFunctions(void) {
	free(funcs);

	return;
}

/*
 *  Create variable
 * 
 *  In: name	Variable name
	      type	Variable type
	      xsize	Size of X subscript
	      ysize	Size of Y subscript
 * 
 *  Returns -1 on error or 0 on success
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
UserDefinedType *usertype;
UserDefinedTypeField *udtfieldptr;
UserDefinedTypeField *fieldptr;
int count;

/* Check if variable name is a reserved name */

if(IsStatement(name) == TRUE) return(INVALID_VARIABLE_NAME);

/* Add entry to variable list */

if(currentfunction->vars == NULL) {			/* first entry */
	 currentfunction->vars=malloc(sizeof(vars_t));		/* add new item to list */
	 if(currentfunction->vars == NULL) return(NO_MEM);	/* can't resize */

	 next=currentfunction->vars;
}
else
{
	next=currentfunction->vars;						/* point to variables */
	last=next;

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
	next->val=calloc(((xsize*MAX_SIZE)+(ysize*MAX_SIZE)),sizeof(char *));
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

	strcpy(next->udt_type,type);		/* set udt type */

	usertype=GetUDT(type);
	if(usertype == NULL) return(INVALID_VARIABLE_TYPE);

	next->udt=malloc((xsize*ysize)*sizeof(UserDefinedType));	/* allocate user-defined type */
	if(next->udt == NULL) return(NO_MEM);

	next->udt->field=malloc(sizeof(UserDefinedTypeField));
	if(next->udt->field == NULL) return(NO_MEM);

	fieldptr=next->udt->field;	
	udtfieldptr=udt->field;
	
	/* copy udt from type definition to variable */

	for(count=0;count<(xsize*ysize);count++) {
		while(udtfieldptr != NULL) {
			/* copy field entries */
		
			strcpy(fieldptr->fieldname,udtfieldptr->fieldname);
			
			fieldptr->type=udtfieldptr->type;
		
			memcpy(fieldptr,udtfieldptr,sizeof(UserDefinedTypeField));	/* copy udt */

			fieldptr->fieldval=malloc(sizeof(vars_t));
			if(fieldptr->fieldval == NULL) return(NO_MEM);

			fieldptr->next=malloc(sizeof(UserDefinedTypeField));		/* allocate field in user-defined type */
			if(fieldptr->next == NULL) return(NO_MEM);

			fieldptr=fieldptr->next;
			udtfieldptr=udtfieldptr->next;

		}			
	}
}

next->xsize=xsize;				/* set size */
next->ysize=ysize;

strcpy(next->varname,name);		/* set name */
next->next=NULL;

return(0);
}

/*
 *  Update variable
 * 
 *  In: char *name	Variable name
	      varval *val	Variable value
	      int x		X subscript
	      int y		Y subscript
 * 
 *  Returns -1 on error or 0 on success
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
UserDefinedTypeField *udtfield;

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) break;		/* already defined */
	 
		next=next->next;
}

if(next == NULL) return(VARIABLE_DOES_NOT_EXIST);

if( ((x*y) > (next->xsize*next->ysize)) || ((x*y) < 0)) {		/* outside array */
	return(INVALID_ARRAY_SUBSCRIPT);
}

/* update variable */


if(next->type_int == VAR_NUMBER) {		/* double precision */			
	      next->val[(y*next->ysize)+(x*next->xsize)].d=val->d;
	      return(0);

}
else if(next->type_int == VAR_STRING) {	/* string */			
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
else if(next->type_int == VAR_INTEGER) {	/* integer */
	      next->val[(y*next->ysize)+(x*next->xsize)].i=val->i;
	      return(0);
}
else if(next->type_int == VAR_SINGLE) {	/* single */
	      next->val[x*sizeof(float)+(y*sizeof(float))].f=val->f;
	      return(0);
}
else {					/* user-defined type */	
	next=GetVariablePointer(name);		/* get variable entry */
	if(next == NULL) return(-1);
	
	udtfield=next->udt->field;

	while(udtfield != NULL) {
		if(strcmp(udtfield->fieldname,fieldname) == 0) {	/* found field */
			val->type=udtfield->type;

			if(udtfield->type == VAR_NUMBER) {
			        udtfield->fieldval[(next->ysize*y)+(next->xsize*x)].d=val->d;
			        return(0);
			 }
			 else if(udtfield->type == VAR_STRING) {
			 	if(udtfield->fieldval[( (y*next->ysize)+(next->xsize*x))].s == NULL) {		/* if string element not allocated */
	 				udtfield->fieldval[((y*next->ysize)+(next->xsize*x))].s=malloc(strlen(val->s));	/* allocate memory */
	      				} 

	      				if( strlen(val->s) > (strlen(udtfield->fieldval[(y*next->ysize)+(next->xsize*x)].s)+(next->xsize*x))) {	/* if string element larger */
	 				realloc(udtfield->fieldval[((y*next->ysize)+(next->xsize*x))].s,strlen(val->s));	/* resize memory */
	      				}
		
			        strcpy(udtfield->fieldval[(next->ysize*y)+(next->xsize*x)].s,val->s);		/* assign value */
			        return(0);
			 }
			 else if(udtfield->type == VAR_INTEGER) {
			        udtfield->fieldval[(y*next->ysize)+(next->xsize*x)].i=val->i;
				return(0);
			 }
			 else if(udtfield->type == VAR_SINGLE) {
			        udtfield->fieldval[(next->ysize*y)+(next->xsize*x)].f=val->f;
				return(0);
			 }
			 else {
				return(-1);
			}	
		 }

		udtfield=udtfield->next;
	}
	
}

return(0);
}

/*
 *  Resize array
 * 
 *  In: char *name	Variable name
	      int x		X subscript
	      int y		Y subscript
 * 
 *  Returns -1 on error or 0 on success
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
 *  Get variable value
 * 
 *  In: char *name	Variable name
	      varname *val	Variable value
 * 
 *  Returns -1 on error or 0 on success
 * 
  */
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy) {
vars_t *next;
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
if( ((x*y) > (next->xsize*next->ysize)) || ((x*y) < 0)) return(INVALID_ARRAY_SUBSCRIPT);	/* outside array */

if(next->type_int == VAR_NUMBER) {
	val->d=next->val[(y*next->ysize)+(x*next->xsize)].d;
	return(0);
}
else if(next->type_int == VAR_STRING) {
	val->s=malloc(strlen(next->val[( (y*next->ysize)+(next->xsize*x))].s));				/* allocate string */
	if(val->s == NULL) return(-1);

	strcpy(val->s,next->val[( (y*next->ysize)+(next->xsize*x))].s);
	return(0);
}
else if(next->type_int == VAR_INTEGER) {
	val->i=next->val[(y*next->ysize)+(x*next->xsize)].i;
	return(0);
}
else if(next->type_int == VAR_SINGLE) {
	val->f=next->val[(y*next->ysize)+(x*next->xsize)].f;
	return(0);
}
else {					/* User-defined type */
	if(GetFieldValueFromUserDefinedType(next->varname,fieldname,val,x,y) == -1) return(-1);
	return(0);
}	

return(INVALID_VARIABLE_TYPE);
}

/*
 *  Get variable type
 * 
 *  In: char *name	Variable name
 * 
 *  Returns -1 on error or variable type on success
 * 
  */

int GetVariableType(char *name) {
vars_t *next;
char c;

if(name == NULL) return(-1);				/* Variable does not exist */

c=*name;

if(c >= '0' && c <= '9') return(VAR_NUMBER);	/* Literal number */

if(c == '"') return(VAR_STRING);			/* Literal string */
	
/* Find variable name */

next=currentfunction->vars;
while(next != NULL) {  
	if(strcmpi(next->varname,name) == 0) return(next->type_int);		/* Found variable */

	next=next->next;
}

return(-1);
}

/*
 *  Split variable into name,subscripts, field and field subscripts
 * 
 *  In: char *name	Variable name
	      varsplit *split	Variable split object
 * 
 *  Returns -1 on error or 0 on success
 * 
  */
int ParseVariableName(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end,varsplit *split) {
int count;
int fieldstart=0;
int fieldend;
int subscriptend;
int commafound=FALSE;
int parse_end=0;

memset(split,0,sizeof(varsplit));

strcpy(split->name,tokens[start]);		/* copy name */

split->x=0;
split->y=0;
split->fieldx=0;
split->fieldy=0;

for(count=start;count<end;count++) {			/* find field start, if any */
	if(strcmp(tokens[count],".") == 0) {
		fieldstart=count+1;
		parse_end += 4;			/* skip name, . and field name */
		break;
	}
}

/* find end of a possible subscript */

if(fieldstart == 0) {
	subscriptend=end;
}

if((strcmp(tokens[start+1],"(") == 0) || (strcmp(tokens[start+1],"[") == 0)) {
	if(strcmp(tokens[start+1],"(") == 0) split->arraytype=ARRAY_SUBSCRIPT;
	if(strcmp(tokens[start+1],"[") == 0) split->arraytype=ARRAY_SLICE;
	
	/* find end of array subscripts */

	for(subscriptend=start+2;count<subscriptend;count++) {
	 	if(strcmp(tokens[subscriptend],")") == 0) break;
	}

	parse_end += (subscriptend-(start+1));

	commafound=FALSE;

	for(count=start+2;count<subscriptend;count++) {
		if(strcmp(tokens[count],",") == 0) {		 /* 3d array */

			if((IsValidExpression(tokens,1,count) == FALSE) || (IsValidExpression(tokens,count+1,subscriptend) == FALSE)) {  /* invalid expression */
				PrintError(SYNTAX_ERROR);
				return(SYNTAX_ERROR);
			}

			split->x=EvaluateExpression(tokens,start+1,count);
			split->y=EvaluateExpression(tokens,count+1,subscriptend);

			commafound=TRUE;
		 	break;
		}
	}

	if(commafound == FALSE) {
		if(IsValidExpression(tokens,start+2,subscriptend+1) == FALSE) {	/* invalid expression */
			PrintError(SYNTAX_ERROR);
			return(SYNTAX_ERROR);
		}

		split->x=EvaluateExpression(tokens,start+2,subscriptend+1);
	 	split->y=1;
	}
	
}

if(fieldstart != 0) {					/* if there is a field name and possible subscripts */
	strcpy(split->fieldname,tokens[fieldstart]);	/* copy field name */

	if((strcmp(tokens[fieldstart+1],"(") == 0) || (strcmp(tokens[fieldstart+1],"[") == 0)) {

		for(fieldend=start+2;count<end;count++) {
			if(strcmp(tokens[fieldend],")") == 0) {
				parse_end=fieldend;
				break;
			}
		}

		for(count=fieldstart+1;count<end;count++) {
	   		if(strcmp(tokens[count],",") == 0) {		 /* 3d array */
				SubstituteVariables(fieldstart+2,count,tokens,tokens);
				SubstituteVariables(count+1,end-1,tokens,tokens);

				if((IsValidExpression(tokens,fieldstart+2,count) == FALSE) || (IsValidExpression(tokens,count+1,end-1) == FALSE)) {  /* invalid expression */
					PrintError(SYNTAX_ERROR);
					return(SYNTAX_ERROR);
				}

				split->fieldx=EvaluateExpression(tokens,fieldstart+2,count);
				split->fieldy=EvaluateExpression(tokens,count+1,end-1);
				break;
			}
	      }

	      if(count == end) {			/* 2d array */  
	      		SubstituteVariables(start+2,end-1,tokens,tokens);

			if(IsValidExpression(tokens,start+2,end-1) == FALSE) {  /* invalid expression */
				PrintError(SYNTAX_ERROR);
				return(SYNTAX_ERROR);
			}

		  	split->fieldx=EvaluateExpression(tokens,start+2,end-1);
		        split->fieldy=1;
	   }
   }
}

return(parse_end);
}

/*
 *  Remove variable
 * 
 *  In: char *name	Variable name
 * 
 *  Returns -1 on error or 0 on success
 * 
  */
int RemoveVariable(char *name) {
vars_t *next;
vars_t *last;


next=currentfunction->vars;						/* point to variables */
	
while(next != NULL) {
//	printf("next=%lX\n",next);

	last=next;
	 
	if(strcmpi(next->varname,name) == 0) {			/* found variable */
		if(next == currentfunction->vars) {		/* first */
			currentfunction->vars=currentfunction->vars->next;
		}
		else if (next->next == NULL) {			/* last */
			last->next=NULL;
		}
		else
		{
	  		last->next=next->next;				/* point over link */
		}

//      	  	free(next);

   		return(0);    
	  }

	 next=next->next;
}

return(-1);
}

/*
 *  Declare function
 * 
 *  In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
	      int funcargcount			number of tokens
 * 
 *  Returns -1 on error or 0 on success
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
				udtptr=GetUDT(tokens[count+2]);
				if(udtptr == NULL) return(INVALID_VARIABLE_TYPE);
				strcpy(vartype,tokens[count+2]);
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

	/* get parameter type */

	if(vartypenames[typecount] != NULL) {			/* built-in type */
		 paramsptr->type_int=typecount;
	}
	else
	{
		paramsptr->type_int=VAR_UDT;
		strcpy(paramsptr->udt_type,tokens[count+2]);	/* user-defined type */
	}

	paramsptr->xsize=0;
	paramsptr->ysize=0;
	paramsptr->val=NULL;

	next->funcstart=GetCurrentBufferPosition();

	if(strcmpi(tokens[count+1], ")") == 0) break;		/* at end */

	if(strcmpi(tokens[count+1], "AS") == 0) {
		count += 3;		/* skip "AS", type and "," */
	}
	else
	{
		count++;		/* skip , */
	}
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
	 if(udtptr == NULL) return(INVALID_VARIABLE_TYPE);
	}

	strcpy(next->returntype,tokens[funcargcount-1]);

/* find end of function */

	do {
	 SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));			/* get data */

	 if(TokenizeLine(linebuf,tokens,TokenCharacters) == -1) {			/* tokenize line */
	  PrintError(SYNTAX_ERROR);
	  return(-1);
	 }

	 if(strcmpi(tokens[0],"ENDFUNCTION") == 0) return;  
	
}    while(*GetCurrentBufferPosition() != 0); 			/* until end */

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
 *  Call function
 * 
 *  In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
	      int funcargcount			number of tokens
 * 
 *  Returns -1 on error or 0 on success
 * 
  */

double CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end) {
functions *next;
int count,countx;
int tc;
char *argbuf[MAX_SIZE][MAX_SIZE];
char *buf[MAX_SIZE];
varsplit split;
varval val;
vars_t *parameters;
int varstc;
int endcount;
FUNCTIONCALLSTACK newfunc;
UserDefinedType userdefinedtype;
int substreturnvalue;

next=funcs;						/* point to variables */
/* find function name */

while(next != NULL) {
	if(strcmpi(next->name,tokens[start]) == 0) break;		/* found name */   

	next=next->next;
}

if(next == NULL) return(INVALID_STATEMENT);

substreturnvalue=SubstituteVariables(start+2,end,tokens,tokens);			/* substitute variables */
if(substreturnvalue > 0) return(substreturnvalue);

/* save information about the calling function. The calling function is already on the stack */

currentfunction->callptr=GetCurrentBufferPosition();

/* save information about the called function */
memset(&newfunc,0,sizeof(FUNCTIONCALLSTACK));

strcpy(newfunc.name,next->name);

newfunc.last=currentfunction;
newfunc.callptr=next->funcstart;

SetCurrentBufferPosition(next->funcstart);

newfunc.linenumber=1;
newfunc.stat |= FUNCTION_STATEMENT;

strcpy(newfunc.returntype,next->returntype);
PushFunctionCallInformation(&newfunc);

currentfunction=&newfunc;

/* add variables from parameters */

parameters=next->parameters;
count=start+2;		/* skip function name and ( */

while(parameters != NULL) {
	if(parameters->type_int == VAR_UDT) {			/* user defined type */
		CreateVariable(parameters->varname,parameters->udt_type,split.x,split.y);
	}
	else							/* built-in type */
	{
		CreateVariable(parameters->varname,vartypenames[parameters->type_int],split.x,split.y);
	}

	if(parameters->type_int == VAR_NUMBER) {
		val.d=atof(tokens[count]);
	}
	else if(parameters->type_int == VAR_STRING) {
		strcpy(val.s,tokens[count]);
	}
	else if(parameters->type_int == VAR_INTEGER) {
		val.i=atoi(tokens[count]);
	}
	else if(parameters->type_int == VAR_SINGLE) {
		val.f=atof(tokens[count]);
	}
	else {
	  	
	}

	UpdateVariable(parameters->varname,NULL,&val,split.x,split.y);

	parameters=parameters->next;

	count += 2;	/* skip , */   
}

/* do function */

while(*GetCurrentBufferPosition() != 0) {	
	SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),buf,LINE_SIZE));			/* get data */

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

ReturnFromFunction();			/* return */

return(0);
}

int ReturnFromFunction(void) {
PopFunctionCallInformation();
}

/*
 *  Convert char * to int using base
 * 
 *  In: char *hex		char representation of number
	      int base			Base
 * 
 *  Returns -1 on error or 0 on success
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
 *  Substitute variable names with values
 * 
 *  In: int start		Start of variables in tokens array
	      int end			End of variables in tokens array
	      char *tokens[][MAX_SIZE] Tokens array
 * 
 *  Returns error number on error or 0 on success
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
char *temp[MAX_SIZE][MAX_SIZE];
varval subst_returnvalue;
int s;
int type;
int skiptokens=0;
int numberofouttokens=0;
int returnvalue;
int arraysize;

outcount=0;

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
	  		numberofouttokens++;
	  	}
	  	else if(retval.val.type == VAR_INTEGER) {		/* returning integer */
			sprintf(temp[outcount++],"%d",retval.val.i);
			numberofouttokens++;
	 	  	}
	  	else if(retval.val.type == VAR_NUMBER) {		/* returning double */
			sprintf(temp[outcount++],"%.6g",retval.val.d);		 		
			numberofouttokens++;	
	  	}
	  	else if(retval.val.type == VAR_SINGLE) {		/* returning single */
			sprintf(temp[outcount++],"%f",retval.val.f);
			numberofouttokens++;
	  	}

	  	count=countx-1;
	 }
	 else if(IsVariable(tokens[count]) == 0) {
	    skiptokens=ParseVariableName(tokens,count,end,&split);	/* split variable name */

	    tokentype=SUBST_VAR;

	    arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));
	    if(((split.x*split.y) > arraysize) || (arraysize < 0)) return(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
	
	    GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy);

	    type=GetVariableType(split.name); 	/* Get variable type */
	    
	    if(type == VAR_UDT) {
		type=GetFieldTypeFromUserDefinedType(split.name,split.fieldname);		/* get field type id udt */
		if(type == -1) return(TYPE_FIELD_DOES_NOT_EXIST);	
	    }

	    switch(type) {
		case VAR_STRING:

		   if(split.arraytype == ARRAY_SLICE) {		/* part of string */
	    	   	arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));
	    		if(((split.x*split.y) > arraysize) || arraysize < 0) return(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
	
			returnvalue=GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy);
		      	if(returnvalue != NO_ERROR) return(returnvalue);

			b=&val.s;			/* get start */
			b += split.x;

			memset(temp[outcount],0,MAX_SIZE);

			d=temp[outcount++];
		 	*d++='"';

			numberofouttokens++;

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
			numberofouttokens++;
		      }
		      else
		      {
	    	      	arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));

	    		if(((split.x*split.y) > arraysize) || arraysize < 0) return(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
	
		        GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy);

		  	sprintf(temp[outcount++],"\"%s\"",val.s);
			numberofouttokens++;

		      }
	      
	            }

		    break;
	
		case VAR_NUMBER:
		    sprintf(temp[outcount++],"%.6g",val.d);
		    numberofouttokens++;

	   	    break;

	        case VAR_INTEGER:	       
		    sprintf(temp[outcount++],"%d",val.i);
	  	    numberofouttokens++;
	    	    break;

	        case VAR_SINGLE:	     
		    sprintf(temp[outcount++],"%f",val.f);
		    numberofouttokens++;
	       	    break;			
	       	}

	 	count += skiptokens;

	 }
	 else
	 {
	  	strcpy(temp[outcount++],tokens[count]);
	 	numberofouttokens++; 
	 }   

	if(count >= end) break;
}

/* copy tokens */

memset(out,0,MAX_SIZE*MAX_SIZE);

for(count=0;count<outcount;count++) {
	 strcpy(out[count],temp[count]);
}

return(0);
}

/*
 *  Conatecate strings
 * 
 *  In: int start		Start of variables in tokens array
	      int end			End of variables in tokens array
	      char *tokens[][MAX_SIZE] Tokens array
	      varval *val 		Variable value to return conatecated strings
 * 
 *  Returns -1 on error or 0 on success
 * 
  */
int ConatecateStrings(int start,int end,char *tokens[][MAX_SIZE],varval *val) {
int count;
char *b;
char *d;

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

 * d++=*b++;
}

for(count=start+1;count<end;count++) {
	if(strcmpi(tokens[count],"+") == 0) { 
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
	return(0);
}

/*
 *  Check variable type
 * 
 *  In: char *typename		Variable type as string
 * 
 *  Returns -1 on error or variable type on success
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
 *  Check if variable
 * 
 *  In: varname		Variable name
 * 
 *  Returns -1 on error or 0 on success
 * 
 */
int IsVariable(char *varname) {
vars_t *next;
varsplit split;
int tc;

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

//printf("push\n");

if(currentfunction == NULL) {
//	printf("first\n");

	currentfunction=malloc(sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */

	if(currentfunction == NULL) {
		PrintError(NO_MEM);
		return(-1);
	}

	currentfunction->last=NULL;
	currentfunction->next=NULL;
	next=currentfunction;
}
else
{
//	printf("next\n");

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
vars_t *vars;

/* remove variables */

vars=currentfunction->vars;

while(vars != NULL) {
	 RemoveVariable(vars->varname);
	 vars=vars->next;
}

currentfunction=currentfunction->last;
SetCurrentBufferPosition(currentfunction->callptr);

//free(currentfunction->next);

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
	UserDefinedType *next;
	UserDefinedType *last;

	if(udt == NULL) {			/* first entry */
		udt=malloc(sizeof(UserDefinedType));		/* add link to the end */
		if(udt == NULL) return(NO_MEM); 

	last=udt;
	}
	else
	{
	/* find end and check if UDT exists */

	next=udt;

		while(next != NULL) {
	 		last=next;

	 		if(strcmpi(next->name,newudt->name) == 0) return(-1);	/* udt exists */

		next=next->next;
	}

	
	/* add link to the end */

	last->next=malloc(sizeof(UserDefinedType));
		if(last->next == NULL) return(NO_MEM); 

	last=last->next;
	}

	memcpy(last,newudt,sizeof(UserDefinedType));

	return(0);
}

int CopyUDT(UserDefinedType *source,UserDefinedType *dest) {
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
	else if(sourcenext->type == VAR_SINGLE) {		/* single precision */			
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

/*
 *  Get field value from user defined type field
 * 
 *  In:
 * 
 *  fieldname	Name of the UDT field to return value of
 * 
 *  val		Value buffer
 *  fieldx	field x subscript
 *  fieldy	field y subscript
 * 
 *  Returns: 0 in success or non-zero error code on failure
 * 
  */

int GetFieldValueFromUserDefinedType(char *varname,char *fieldname,varval *val,int fieldx,int fieldy) {
UserDefinedTypeField *udtfield;
vars_t *next;

next=GetVariablePointer(varname);		/* get variable entry */
if(next == NULL) return(-1);

udtfield=next->udt->field;

while(udtfield != NULL) {

	if(strcmp(udtfield->fieldname,fieldname) == 0) {	/* found field */				
		val->type=udtfield->type;
		if(udtfield->type == VAR_NUMBER) {
		        val->d=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].d;
		        return(0);
		 }
		 else if(udtfield->type == VAR_STRING) {
			val->s=malloc(strlen(udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s)+1);	/* allocate string */
			if(val->s == NULL) return(-1);

		        strcpy(val->s,udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s);
		        return(0);
		 }
		 else if(udtfield->type == VAR_INTEGER) {
		        val->i=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].i;
			return(0);
		 }
		 else if(udtfield->type == VAR_SINGLE) {
		        val->f=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].f;
			return(0);
		 }
		 else {
			return(-1);
		}	
	 }

	udtfield=udtfield->next;
	}

return(-1);
}	

/*
*  Check if valid variable type
* 
*  In: type	variable type name
* 
*  Returns 0 if valid type, -1 otherwise
* 
*/

int IsValidVariableType(char *type) {
int count=0;

while(vartypenames[count] != NULL) {
	if(strcmpi(vartypenames[count],type) == 0) return(count);

	count++;
}

return(-1);
}

/*
 *  Get field type from user defined type field
 * 
 *  In: *  varname	Variable
 *  fieldname	Name of the UDT field to return value of
 * 
 *  val		Value buffer
 *  x		x subscript
 *  y		y subscript
 * 
 *  Returns: field type on success or -1 on failure
 * 
  */

int GetFieldTypeFromUserDefinedType(char *varname,char *fieldname) {
UserDefinedTypeField *udtfield;
vars_t *next;

next=GetVariablePointer(varname);		/* get variable entry */
if(next == NULL) return(-1);

udtfield=next->udt->field;

while(udtfield != NULL) {
	if(strcmpi(udtfield->fieldname,fieldname) == 0) return(udtfield->type);

	udtfield=udtfield->next;
}

return(-1);
}

/*
*  Get current function name
* 
*  In: buffer for name
* 
*  Returns: nothing
* 
*/

void GetCurrentFunctionName(char *buf) {
strcpy(buf,currentfunction->name);
}

/*
*  Get current function line number
* 
*  In: Nothing
* 
*  Returns: Line number
* 
*/

int GetCurrentFunctionLine(void) {
return(currentfunction->linenumber);
}

/*
*  Set current function line number
* 
*  In: Line number
* 
*  Returns: Nothing
* 
*/

void SetCurrentFunctionLine(int linenumber) {
currentfunction->linenumber=linenumber;
}

/*
*  Set current function call pointer
* 
*  In: Call pointer
* 
*  Returns: Nothing
* 
*/
void SetFunctionCallPtr(char *ptr) {
currentfunction->callptr=ptr;
}

/*
*  Set current function flags
* 
*  In: Flags
* 
*  Returns: Nothing
* 
*/
void SetFunctionFlags(int flags) {
currentfunction->stat |= flags;
}

/*
*  Clear current function flags
* 
*  In: Flags
* 
*  Returns: Nothing
* 
*/
void ClearFunctionFlags(int flags) {
currentfunction->stat |= ~flags;
}

/*
*  Get current function flags
* 
*  In: Nothing
* 
*  Returns: flags
* 
*/
int GetFunctionFlags(void) {
return(currentfunction->stat);
}

/*
*  Get save information buffer pointer
*
*  In: Nothing
* 
*  Returns: buffer pointer
* 
*/
char *GetSaveInformationBufferPointer(void) {
char *ptr;

//if(currentfunction->saveinformation_top->bufptr == NULL) return(NULL);

ptr=currentfunction->saveinformation_top->bufptr;
return(ptr);
}

/*
*  Get save information buffer pointer
*
*  In: Nothing
* 
*  Returns: buffer pointer
* 
*/
int GetSaveInformationLineCount(void) {
SAVEINFORMATION *info;

info=currentfunction->saveinformation_top;
if(info == NULL) return(NULL);

return(info->linenumber);
}


/*
*  Get current function return variable type
* 
*  In: Nothing
* 
*  Returns: return variable type
* 
*/

int GetFunctionReturnType(void) {
return(currentfunction->type_int);
}

int PushSaveInformation(void) {
SAVEINFORMATION *info;
SAVEINFORMATION *previousinfo;

if(currentfunction->saveinformation_top == NULL) {
	currentfunction->saveinformation_top=malloc(sizeof(SAVEINFORMATION));		/* allocate new entry */

	if(currentfunction->saveinformation_top == NULL) {
		PrintError(NO_MEM);
		return(NO_MEM);
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
		return(NO_MEM);
	}

	info=previousinfo->next;
	info->last=previousinfo;					/* link previous to next */
	
}

info->bufptr=GetCurrentBufferPosition();
info->linenumber=GetCurrentFunctionLine();
info->next=NULL;
}

int PopSaveInformation(void) {
SAVEINFORMATION *info;
SAVEINFORMATION *previousinfo;

if(currentfunction->saveinformation_top->last != NULL) {
	info=currentfunction->saveinformation_top;

	previousinfo=info;
	currentfunction->saveinformation_top=info->last;		/* point to previous */

	free(previousinfo);
}
else
{
	SetCurrentBufferPosition(currentfunction->saveinformation_top->bufptr);
	currentfunction->linenumber=currentfunction->saveinformation_top->linenumber;
}

}

/*
*  Get variable X size
* 
*  In: Nothing
* 
*  Returns: return variable type
* 
*/

int GetVariableXSize(char *name) {
vars_t *next;

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) return(next->xsize);
	 
	next=next->next;
}

return(-1);
}

/*
*  Get variable Y size
* 
*  In: Nothing
* 
*  Returns: return variable type
* 
*/

int GetVariableYSize(char *name) {
vars_t *next;

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) return(next->ysize);
	 
	next=next->next;
}

return(-1);
}

