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
#include "module.h"
#include "variablesandfunctions.h"
#include "errors.h"
#include "dofile.h"
#include "evaluate.h"
#include "debugmacro.h"

extern char *TokenCharacters;

functions *funcs=NULL;
functions *funcs_end=NULL;
FUNCTIONCALLSTACK *functioncallstack=NULL;
FUNCTIONCALLSTACK *FunctionCallStackTop=NULL;
FUNCTIONCALLSTACK *currentfunction=NULL;
UserDefinedType *udt=NULL;
char *vartypenames[] = { "DOUBLE","STRING","INTEGER","SINGLE","LONG",NULL };
functionreturnvalue retval;
vars_t *findvar;
int callpos=0;

/*
 * Intialize main function
 * 
 *  In: args	command-line arguments
 * 
 *  Returns: Nothing
 * 
 */
void InitializeMainFunction(char *progname,char *args) {
FUNCTIONCALLSTACK newfunc;
functions mainfunc;
char *tokens[MAX_SIZE][MAX_SIZE];
int NextToken=2;
int whichreturntype;

/* Add main function to list of functions */

strcpy(tokens[0],"main");
strcpy(tokens[1],"(");
if(args != NULL) strcpy(tokens[NextToken++],args);
strcpy(tokens[NextToken],")");

DeclareFunction(tokens,4);			/* declare main function */

/* push main function onto call stack */
strcpy(newfunc.name,"main");

newfunc.callptr=NULL;
newfunc.startlinenumber=1;
newfunc.currentlinenumber=1;
newfunc.saveinformation=NULL;
newfunc.saveinformation_top=NULL;
newfunc.parameters=NULL;
newfunc.vars=NULL;
newfunc.vars_end=NULL;
newfunc.stat=0;
newfunc.moduleptr=GetCurrentModuleInformationFromBufferAddress(GetCurrentBufferAddress());

strcpy(newfunc.returntype,vartypenames[DEFAULT_TYPE_INT]);

newfunc.type_int=DEFAULT_TYPE_INT;
newfunc.lastlooptype=0;
newfunc.next=NULL;

PushFunctionCallInformation(&newfunc);

DeclareBuiltInVariables(progname,args);			/* declare built-in variables */
}

/*
 * Declare built-in variables
 * 
 * In: args	command-line arguments
 * 
 * Returns: Nothing
 * 
 */
void DeclareBuiltInVariables(char *progname,char *args) {
varval cmdargs;

cmdargs.s=malloc(MAX_SIZE);

CreateVariable("PROGRAMNAME","STRING",1,1);
CreateVariable("COMMAND","STRING",1,1);

/* get program name */

if(progname != NULL) {
	strcpy(cmdargs.s,progname);
	UpdateVariable("PROGRAMNAME","",&cmdargs,1,1,0,0);
}

/* get command-line arguments, if any */

if(args != NULL) {
	strcpy(cmdargs.s,args);
	UpdateVariable("COMMAND","",&cmdargs,1,1,0,0);
}

/* add built-in variables */

cmdargs.i=0;

CreateVariable("ERR","INTEGER",1,1);			/* error number */
UpdateVariable("ERR","",&cmdargs,1,1,0,0);

CreateVariable("ERRL","INTEGER",1,1);			/* error line */
UpdateVariable("ERRL","",&cmdargs,1,1,0,0);

CreateVariable("ERRFUNC","STRING",1,1);			/* error function */
UpdateVariable("ERRFUNC","",&cmdargs,1,1,0,0);

free(cmdargs.s);

udt=NULL;		/* set pointers to null */
funcs=NULL;
funcs_end=NULL;
}

/*
 *  Create variable
 * 
 *  In: name	Variable name
	      type	Variable type
	      xsize	Size of X subscript
	      ysize	Size of Y subscript
 * 
 *  Returns -1 on failure or 0 on success
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

if(currentfunction == NULL) return(-1);

/* Check if variable name is a reserved name */

if(IsStatement(name)) {
	SetLastError(INVALID_VARIABLE_NAME);
	return(-1);
}

if(IsVariable(name)) {
	SetLastError(VARIABLE_EXISTS);
	return(-1);
}

/* Add entry to variable list */

if(currentfunction->vars == NULL) {			/* first entry */
	currentfunction->vars=malloc(sizeof(vars_t));		/* add new item to list */
	if(currentfunction->vars == NULL) SetLastError(NO_MEM);

	currentfunction->vars_end=currentfunction->vars;
	currentfunction->vars_end->last=NULL;
	
}
else
{
	currentfunction->vars_end->next=malloc(sizeof(vars_t));		/* add new item to list */
	if(currentfunction->vars_end->next == NULL) SetLastError(NO_MEM);

	currentfunction->vars_end=currentfunction->vars_end->next;
}

/* add to end */

if(strcmpi(type,"DOUBLE") == 0) {		/* double precision */			
	currentfunction->vars_end->val=malloc((xsize*sizeof(double))*(ysize*sizeof(double)));

	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=VAR_NUMBER;
}
else if(strcmpi(type,"STRING") == 0) {	/* string */

	currentfunction->vars_end->val=calloc(((xsize*MAX_SIZE)+(ysize*MAX_SIZE)),sizeof(char *));

	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=VAR_STRING;
}
else if(strcmpi(type,"INTEGER") == 0) {	/* integer */
	currentfunction->vars_end->val=malloc((xsize*sizeof(int))*(ysize*sizeof(int)));

	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=VAR_INTEGER;
}
else if(strcmpi(type,"SINGLE") == 0) {	/* single */
	currentfunction->vars_end->val=malloc((xsize*sizeof(float))*(ysize*sizeof(float)));

	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=VAR_SINGLE;
}
else if(strcmpi(type,"LONG") == 0) {	/* long */
	currentfunction->vars_end->val=malloc((xsize*sizeof(long int))*(ysize*sizeof(long)));
	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=VAR_LONG;
}
else {					/* user-defined type */	 
	currentfunction->vars_end->type_int=VAR_UDT;

	strcpy(currentfunction->vars_end->udt_type,type);		/* set udt type */

	usertype=GetUDT(type);
	if(usertype == NULL) {
		SetLastError(INVALID_VARIABLE_TYPE);
		return(-1);
	}

	currentfunction->vars_end->udt=malloc((xsize*ysize)*sizeof(UserDefinedType));	/* allocate user-defined type */
	if(currentfunction->vars_end->udt == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	strcpy(currentfunction->vars_end->udt,type);		/* Copy type */

	currentfunction->vars_end->udt->field=malloc(sizeof(UserDefinedTypeField));
	if(currentfunction->vars_end->udt->field == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
	
	fieldptr=currentfunction->vars_end->udt->field;	
	udtfieldptr=udt->field;
	
	/* copy udt from type definition to variable */

	for(count=0;count<((xsize+1)*(ysize+1));count++) {
		while(udtfieldptr != NULL) {
			/* copy field entries */
		
			memcpy(fieldptr,udtfieldptr,sizeof(UserDefinedTypeField));	/* copy udt */

			strcpy(fieldptr->fieldname,udtfieldptr->fieldname);
			fieldptr->type=udtfieldptr->type;
		
			fieldptr->fieldval=malloc(sizeof(vars_t));
			if(fieldptr->fieldval == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

			fieldptr->next=malloc(sizeof(UserDefinedTypeField));		/* allocate field in user-defined type */
			if(fieldptr->next == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

			fieldptr=fieldptr->next;			udtfieldptr=udtfieldptr->next;

		}			
	}
}

currentfunction->vars_end->xsize=xsize;				/* set size */
currentfunction->vars_end->ysize=ysize;

strcpy(currentfunction->vars_end->varname,name);		/* set name */
currentfunction->vars_end->next=NULL;

SetLastError(0);
return(0);
}

/*
 *  Update variable
 * 
 *  In: name	Variable name
	val	Variable value
	x	X subscript
	y	Y subscript
 * 
 *  Returns -1 on failure or 0 on success
 * 
 */
int UpdateVariable(char *name,char *fieldname,varval *val,int x,int y,int fieldx,int fieldy) {
vars_t *next;
char *o;
varval *varv;
char *strptr;
varsplit split;
UserDefinedType *udtptr;
UserDefinedTypeField *fieldptr;
UserDefinedTypeField *udtfield;

if(currentfunction == NULL) return(-1);

if(currentfunction != NULL) {
	next=currentfunction->vars;

	while(next != NULL) {
		if(strcmpi(next->varname,name) == 0) break;		/* already defined */
	 
		next=next->next;
	}
}

if(next == NULL) return(-1);

if( ((x*y) > (next->xsize*next->ysize)) || ((x*y) < 0)) return(-1);	/* outside array */

/* update variable */

if(next->type_int == VAR_NUMBER) {		/* double precision */	
	      next->val[(y*next->ysize)+x].d=val->d;

	      return(0);
}
else if(next->type_int == VAR_STRING) {	/* string */
	if(next->val[( (y*next->ysize)+x)].s == NULL) {		/* if string element not allocated */
		next->val[((y*next->ysize)+x)].s=malloc(strlen(val->s));	/* allocate memory */
		strcpy(next->val[((y*next->ysize)+x)].s,val->s);		/* assign value */
	} 

	if( strlen(val->s) > strlen(next->val[( (y*next->ysize)+x)].s)) {	/* if string element larger */
		realloc(next->val[((y*next->ysize)+x)].s,strlen(val->s));	/* resize memory */
		strcpy(next->val[((y*next->ysize)+x)].s,val->s);		/* assign value */
	}

	return(0);
}
else if(next->type_int == VAR_INTEGER) {	/* integer */
	next->val[(y*next->ysize)+x].i=val->i;

	return(0);
}
else if(next->type_int == VAR_SINGLE) {	/* single */
	next->val[x*sizeof(float)+(y*sizeof(float))].f=val->f;

	return(0);
}
else if(next->type_int == VAR_LONG) {	/* long */
	next->val[x*sizeof(long int)+(y*sizeof(long int))].l=val->l;

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
			        udtfield->fieldval[(udtfield->ysize*y)+fieldx].d=val->d;

				return(0);
			 }
			 else if(udtfield->type == VAR_STRING) {
			 	if(udtfield->fieldval[( (y*udtfield->ysize)+x)].s == NULL) {		/* if string element not allocated */
	 				udtfield->fieldval[((y*udtfield->ysize)+fieldx)].s=malloc(strlen(val->s));	/* allocate memory */
	      			} 

	      			if( strlen(val->s) > (strlen(udtfield->fieldval[(y*udtfield->ysize)+fieldx].s)+x)) {	/* if string element larger */
	 				realloc(udtfield->fieldval[((fieldy*udtfield->ysize)+fieldx)].s,strlen(val->s));	/* resize memory */
	      			}
		
			        strcpy(udtfield->fieldval[(udtfield->ysize*fieldy)+fieldx].s,val->s);		/* assign value */
				return(0);
			 }
			 else if(udtfield->type == VAR_INTEGER) {
			        udtfield->fieldval[(fieldy*udtfield->ysize)+fieldx].i=val->i;
				return(0);
			 }
			 else if(udtfield->type == VAR_SINGLE) {
			        udtfield->fieldval[(udtfield->ysize*fieldy)+fieldx].f=val->f;
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
 *  Returns -1 on failure or 0 on success
 * 
  */								
int ResizeArray(char *name,int x,int y) {
vars_t *next;
char *o;
int statementcount;

if(currentfunction == NULL) return(-1);

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {

	if(strcmpi(next->varname,name) == 0) {		/* found variable */

		if(next->type_int == VAR_UDT) {		/* if user-defined type */
			if(realloc(next->udt,(x*y)*sizeof(UserDefinedType)) == NULL) {	/* resize variable */
				SetLastError(NO_MEM);	
				return(-1);
			}
	  	}
		else
		{
			if(realloc(next->val,(x*y)*sizeof(varval)) == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}
		}
  
		next->xsize=x;		/* update x subscript */
		next->ysize=y;		/* update y subscript */

		SetLastError(0);
		return(0);
 	}

	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Get variable value
 * 
 *  In: char *name	Variable name
	      varname *val	Variable value
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy) {
vars_t *next;
UserDefinedTypeField *udtfield;

if(currentfunction == NULL) return(-1);

if(name == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

if(((char) *name >= '0') && ((char) *name <= '9')) {
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

	      case VAR_LONG:				/* long */
	      	val->l=atol(name);
		break;

	      default:
	      	val->d=atof(name);
		break;
	   }

	SetLastError(0);
}

if((char) *name == '"') {
	val->s=malloc(strlen(name));				/* allocate string */
	if(val->s == NULL) SetLastError(NO_MEM);

	strcpy(val->s,name);
	SetLastError(0);
}

/* Find variable */

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) break;		/* already defined */	 
	next=next->next;
}

if(next == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

if((x > 1) || (y > 1)) {
	if( (x > next->xsize) || (y > next->ysize)) {
		SetLastError(INVALID_ARRAY_SUBSCRIPT);	/* outside array */
		return(-1);
	}
}

if(next->type_int == VAR_NUMBER) {
	val->d=next->val[(y*next->ysize)+x].d;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_STRING) {
	val->s=malloc(strlen(next->val[( (y*next->ysize)+x)].s));				/* allocate string */
	if(val->s == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	strcpy(val->s,next->val[( (y*next->ysize)+x)].s);

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_INTEGER) {
	val->i=next->val[(y*next->ysize)+x].i;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_SINGLE) {
	val->f=next->val[(y*next->ysize)+x].f;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_LONG) {
	val->l=next->val[(y*next->ysize)+x].l;

	SetLastError(0);
	return(0);
}
else {					/* User-defined type */
	if(GetFieldValueFromUserDefinedType(next->varname,fieldname,val,fieldx,fieldy) == -1) SetLastError(-1);

	SetLastError(0);
	return(0);
}	

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Get variable type
 * 
 *  In: char *name	Variable name
 * 
 *  Returns -1 on failure or variable type on success
 * 
  */

int GetVariableType(char *name) {
vars_t *next;

if(currentfunction == NULL) return(-1);

if(name == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

if(((char) *name >= '0') && ((char) *name <= '9')) return(VAR_NUMBER);	/* Literal number */

if((char) *name == '"') return(VAR_STRING);			/* Literal string */
	
/* Find variable name */

next=currentfunction->vars;
while(next != NULL) {  
	if(strcmpi(next->varname,name) == 0) return(next->type_int);		/* Found variable */

	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Split variable into name,subscripts, field and field subscripts
 * 
 *  In: char *name	Variable name
	      varsplit *split	Variable split object
 * 
 *  Returns -1 on failure or number of tokens parsed on success
 * 
  */
int ParseVariableName(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end,varsplit *split) {
int count;
int fieldstart=0;
int fieldend;
int subscriptstart;
int subscriptend;
int commafound=FALSE;
char ParseEndChar;
char *evaltokens[MAX_SIZE][MAX_SIZE];
int evaltc;
int varend;
int tokencount=0;

memset(split,0,sizeof(varsplit));

strcpy(split->name,tokens[start]);		/* copy name */

split->x=0;
split->y=0;
split->fieldx=0;
split->fieldy=0;

for(fieldstart=end;fieldstart > start;fieldstart--) {		/* find field start, if any */
	if(strcmp(tokens[fieldstart],".") == 0) {
		fieldstart++;
		tokencount++;
		break;
	}
}

if((strcmp(tokens[start+1],"(") == 0) || (strcmp(tokens[start+1],"[") == 0)) {
	subscriptstart=start+1;

	if(strcmp(tokens[start+1],"(") == 0) split->arraytype=ARRAY_SUBSCRIPT;
	if(strcmp(tokens[start+1],"[") == 0) split->arraytype=ARRAY_SLICE;
	
	for(subscriptend=end;subscriptend>start+1;subscriptend--) {
		tokencount++;

		if(strcmp(tokens[subscriptend],")") == 0) break;
	}

	/* find array x and y values */
	commafound=FALSE;

	for(count=start+1;count<end;count++) {

			/* Skip commas in arrays and function calls */

			if((strcmp(tokens[count],"(") == 0) || (strcmp(tokens[count],"[") == 0)) {

				if(strcmp(tokens[count],"(") == 0) ParseEndChar=')';
				if(strcmp(tokens[count],"[") == 0) ParseEndChar=']';

				varend=count;
				while(*tokens[varend] != ParseEndChar) {
					if(varend == end) {		/* Missing end */
						SetLastError(SYNTAX_ERROR);
						return(-1);
					}

					varend++;
				}
			}

			if(strcmp(tokens[count],",") == 0) {		 /* 3D array */
				/* invalid expression */
				if((IsValidExpression(tokens,subscriptstart+1,count-1) == FALSE) || (IsValidExpression(tokens,count+1,subscriptend-1) == FALSE)) {
					SetLastError(INVALID_EXPRESSION);
					return(-1);
				}
	
				evaltc=SubstituteVariables(subscriptstart,count,tokens,evaltokens);
				split->x=EvaluateExpression(evaltokens,0,evaltc);

				evaltc=SubstituteVariables(count+1,subscriptend,tokens,evaltokens);
				split->y=EvaluateExpression(evaltokens,0,evaltc);

				commafound=TRUE;
			 	break;
			}
	}

	if(commafound == FALSE) {			/* 2d array */
		if(IsValidExpression(tokens,subscriptstart+1,subscriptend-1) == FALSE) return(-1);	/* invalid expression */

		evaltc=SubstituteVariables(subscriptstart,subscriptend,tokens,evaltokens);

		split->x=EvaluateExpression(evaltokens,0,evaltc);
	 	split->y=1;
	}
	
}

if(fieldstart != start) {					/* if there is a field name and possible subscripts */
	strcpy(split->fieldname,tokens[fieldstart]);	/* copy field name */
	tokencount++;

	if((strcmp(tokens[fieldstart+1],"(") == 0) || (strcmp(tokens[fieldstart+1],"[") == 0)) {

		for(fieldend=start+2;count<end;count++) {
			tokencount++;			
			if(strcmp(tokens[fieldend],")") == 0) break;

		}

		for(count=fieldstart+1;count<end;count++) {
	   		if(strcmp(tokens[count],",") == 0) {		 /* 3d array */
				evaltc=SubstituteVariables(fieldstart+2,count,tokens,evaltokens);
				split->fieldx=EvaluateExpression(tokens,0,evaltc);

				SubstituteVariables(count+1,subscriptend-1,tokens,evaltokens);
				split->fieldy=EvaluateExpression(evaltokens,0,evaltc);

				if((IsValidExpression(tokens,fieldstart+2,count) == FALSE) || (IsValidExpression(tokens,count+1,end-1) == FALSE)) {  /* invalid expression */
					SetLastError(INVALID_EXPRESSION);
					return(-1);
				}

			
				break;
			}
	      }

	      if(count == end) {			/* 2d array */  
		      	evaltc=SubstituteVariables(start+2,end-1,tokens,evaltokens);

			if(IsValidExpression(tokens,start+2,end-1) == FALSE) {  /* invalid expression */
				SetLastError(INVALID_EXPRESSION);
				return(-1);
			}

		  	split->fieldx=EvaluateExpression(evaltokens,0,evaltc);
		        split->fieldy=1;
	   }
   }
}

return(tokencount);
}

/*
 *  Remove variable
 * 
 *  In: char *name	Variable name
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */
int RemoveVariable(char *name) {
vars_t *next;

if(currentfunction == NULL) return(-1);

next=currentfunction->vars;						/* point to variables */
	
while(next != NULL) {

	if(strcmpi(next->varname,name) == 0) {			/* found variable */
		if(next == currentfunction->vars) {		/* first */
			currentfunction->vars=currentfunction->vars->next;

			free(next);
		}
		else if (next->next == NULL) {			/* last */
			free(next);

			next=NULL;
		}
		else
		{
	  		next->last=next->next->last;
			free(next);
		}

   		SetLastError(0);
		return(0);   
	  }

	 next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Declare function
 * 
 *  In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
	      int funcargcount			number of tokens
 * 
 *  Returns -1 on failure or 0 on success
 * 
 */
int DeclareFunction(char *tokens[MAX_SIZE][MAX_SIZE],int end) {
functions *next;
functions *previous;
char *linebuf[MAX_SIZE];
int count;
vars_t *varptr;
vars_t *paramsptr;
vars_t *paramslast;
int typecount=0;
char *vartype[MAX_SIZE];
UserDefinedType *udtptr;
UserDefinedTypeField *udtfieldptr;
int NumberOfParameters=0;

if((currentfunction != NULL) && ((currentfunction->stat & FUNCTION_STATEMENT) == FUNCTION_STATEMENT)) {
	SetLastError(NESTED_FUNCTION);
	return(-1);
}

if(IsFunction(tokens[0]) == TRUE) {	/* Check if function already exists */
	SetLastError(FUNCTION_EXISTS);
	return(-1);
}

if(funcs == NULL) {
	funcs=malloc(sizeof(functions));		/* add new item to list */

	if(funcs == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	funcs_end=funcs;	funcs_end->last=NULL;
}
else
{
	funcs_end->next=malloc(sizeof(functions));		/* add new item to list */
	if(funcs_end->next == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	previous=funcs_end;
	funcs_end=funcs_end->next;

	funcs_end->last=previous;		/* point to previous entry */
}

strcpy(funcs_end->name,tokens[0]);				/* copy name */

funcs_end->funcstart=GetCurrentBufferPosition();
if(currentfunction == NULL) {
	funcs_end->linenumber=1;
}
else
{
	funcs_end->linenumber=currentfunction->currentlinenumber;
}

/* skip ( and go to end */

for(count=2;count < end;count++) {
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
			if(udtptr == NULL) {
				SetLastError(INVALID_VARIABLE_TYPE);
				return(-1);
			}

			strcpy(vartype,tokens[count+2]);
		}
	}
	  
/* add parameter */

	if(funcs_end->parameters == NULL) {
		funcs_end->parameters=malloc(sizeof(vars_t));
		paramsptr=funcs_end->parameters;
	}
	else
	{
		paramsptr=funcs_end->parameters;
		while(paramsptr != NULL) {
	  		paramslast=paramsptr;
	
	  		paramsptr=paramsptr->next;
	  	}
	
		paramslast->next=malloc(sizeof(vars_t));		/* add to end */
	  	paramsptr=paramslast->next;
	}

	/* add function parameters */

	if(vartypenames[typecount] != NULL) {			/* built-in type */
		paramsptr->type_int=typecount;
	}
	else
	{
		paramsptr->type_int=VAR_UDT;
		strcpy(paramsptr->udt_type,tokens[count+2]);	/* user-defined type */
	}

	strcpy(paramsptr->varname,tokens[count]);

	paramsptr->xsize=0;
	paramsptr->ysize=0;
	paramsptr->val=NULL;

	NumberOfParameters++;

	if(strcmpi(tokens[count+1], ")") == 0) break;		/* at end */

	if(strcmpi(tokens[count+1], "AS") == 0) {
		count += 3;		/* skip "AS", type and "," */
	}
	else
	{
		count++;		/* skip , */
	}
}

funcs_end->funcargcount=NumberOfParameters;

if(GetInteractiveModeFlag()) funcs_end->WasDeclaredInInteractiveMode == TRUE;	/* function was declared in interactive mode */

/* get function return type */

typecount=0;	

if(strcmpi(tokens[end-1], "AS") == 0) {		/* type */
	while(vartypenames[typecount] != NULL) {
		if(strcmpi(vartypenames[typecount],tokens[count+1]) == 0) break;	/* found type */

		typecount++;
	}

	if(vartypenames[typecount] == NULL) {		/* possible user-defined type */
		udtptr=GetUDT(tokens[count+1]);
		if(udtptr == NULL) {			/* not user-defined type */
			SetLastError(TYPE_DOES_NOT_EXIST);
			return(-1);
		}

		strcpy(funcs_end->returntype,tokens[count+1]);	
	}
}
else
{
	strcpy(funcs_end->returntype,vartypenames[DEFAULT_TYPE_INT]);
}

if((strcmp(tokens[0],"main") == 0) || GetInteractiveModeFlag()) {		/* special case for main() or interactive mode */
	SetLastError(0);
	return(0);
}
else
{
	if(FindEndOfFunction() == -1) return(-1);		/* find end of declared function */
}

SetLastError(0);
return(0);
}

/*
 *  Find end of function
 * 
 *  In: Nothing
 * 
 *  Returns: 0 on success or -1 on failure
 * 
  */
int FindEndOfFunction(void) {
char *linebuf[MAX_SIZE];
char *tokens[MAX_SIZE][MAX_SIZE];
int lc;

/* find end of function */

do {	
	SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),linebuf,LINE_SIZE));			/* get data */	

	removenewline(linebuf);		/* remove newline */

	TokenizeLine(linebuf,tokens,TokenCharacters);			/* tokenize line */

	if(strcmpi(tokens[0],"ENDFUNCTION") == 0) {		/* found end of function */
		SetLastError(0);  
		return(0);
	}

}  while(*GetCurrentBufferPosition() != 0); 			/* until end */

SetLastError(FUNCTION_NO_ENDFUNCTION);
return(-1);
}

int CheckFunctionExists(char *name) {
functions *next;

next=funcs;						/* point to variables */

/* find function name */

while(next != NULL) {
	if(strcmpi(next->name,name) == 0) {
		SetLastError(0);		/* found name */
		return(0);
	}

	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}


/*
 *  Call function
 * 
 *  In: char *tokens[MAX_SIZE][MAX_SIZE] function name and args
	      int funcargcount			number of tokens
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */

int CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end) {
functions *next;
int count;
int tc;
char *argbuf[MAX_SIZE][MAX_SIZE];
char *buf[MAX_SIZE];
varsplit split;
vars_t *parameters;
int varstc;
FUNCTIONCALLSTACK newfunc;
UserDefinedType userdefinedtype;
int returnvalue;
char *evaltokens[MAX_SIZE][MAX_SIZE];
int linenumber;
int NumberOfArguments=0;

retval.has_returned_value=FALSE;			/* clear has returned value flag */

next=funcs;						/* point to variables */
/* find function name */

while(next != NULL) {
	if(strcmpi(next->name,tokens[start]) == 0) break;		/* found name */   

	next=next->next;
}

if(next == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

returnvalue=SubstituteVariables(start+2,end,tokens,evaltokens);			/* substitute variables */
if(returnvalue == -1) return(-1);

/* save information about the calling function. The calling function is already on the stack */

currentfunction->callptr=GetCurrentBufferPosition();

/* save information about the called function */

strcpy(newfunc.name,next->name);			/* function name */newfunc.callptr=next->funcstart;			/* function start */

SetCurrentBufferPosition(next->funcstart);

newfunc.startlinenumber=next->linenumber;
newfunc.currentlinenumber=next->linenumber;
newfunc.stat |= FUNCTION_STATEMENT;
newfunc.moduleptr=GetCurrentModuleInformationFromBufferAddress(next->funcstart);		/* get module information for this function */

strcpy(newfunc.returntype,next->returntype);

PushFunctionCallInformation(&newfunc);			/* push function information onto call stack */

currentfunction=FunctionCallStackTop;			/* point to function */
currentfunction->parameters=next->parameters;		/* point to parameters */

//DeclareBuiltInVariables("","");				/* add built-in variables */

/* add variables from parameters */

parameters=next->parameters;

for(count=0;count<returnvalue;count += 2) {
	if(parameters->type_int == VAR_UDT) {			/* user defined type */
		CreateVariable(parameters->varname,parameters->udt_type,split.x,split.y);
	}
	else							/* built-in type */
	{
		CreateVariable(parameters->varname,vartypenames[parameters->type_int],split.x,split.y);
	}

	parameters->val=malloc(sizeof(vars_t));		/* allocate variable value */
	if(parameters->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	if(parameters->type_int == VAR_NUMBER) {
		parameters->val->d=atof(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_STRING) {
		strcpy(parameters->val->s,evaltokens[count]);
	}
	else if(parameters->type_int == VAR_INTEGER) {
		parameters->val->i=atoi(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_SINGLE) {
		parameters->val->f=atof(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_LONG) {
		parameters->val->l=atol(evaltokens[count]);
	}
	else {
	  	
	}

	UpdateVariable(parameters->varname,NULL,parameters->val,split.x,split.y,split.fieldx,split.fieldy);

	NumberOfArguments++;

	parameters=parameters->next;
	if(parameters == NULL) break;	
}

/* check if number of arguments matches number of parameters */

if( (NumberOfArguments < next->funcargcount) || ((returnvalue/2) > NumberOfArguments)) {
	SetLastError(INVALID_ARGUMENT_COUNT);
	return(-1);
}

/* call function */

linenumber=next->linenumber;

while(*GetCurrentBufferPosition() != 0) {	
	SetCurrentBufferPosition(ReadLineFromBuffer(GetCurrentBufferPosition(),buf,LINE_SIZE));		/* read line from buffer */

	tc=TokenizeLine(buf,argbuf,TokenCharacters);			/* tokenize line */

	if(strcmpi(argbuf[0],"ENDFUNCTION") == 0) break;

	SetCurrentFunctionLine(++linenumber);			/* set function line number */

	returnvalue=ExecuteLine(buf);				/* Run line */
	if(returnvalue == -1) return(-1);

	if(strcmpi(argbuf[0],"RETURN") == 0) break;
}

currentfunction->stat &= FUNCTION_STATEMENT;

ReturnFromFunction();			/* set script's function return value */

if(returnvalue != -1) SetLastError(0);

return(returnvalue);
}

/*
 *  Return from function
 * 
 *  In: Nothing
 * 
 *  Returns: Nothing
 * 
 */
void ReturnFromFunction(void) {
PopFunctionCallInformation();
}

/*
 *  Convert char * to int using base
 * 
 *  In: char *hex		char representation of number
 *	      int base			Base
 * 
 *  Returns -1 on failure or 0 on success
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
 *  Returns -1 on error or number of substituted tokens on success
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
int outcount=0;
varval val;
int tokentype;
char *temp[MAX_SIZE][MAX_SIZE];
varval subst_returnvalue;
int s;
int type;
int skiptokens=0;
int returnvalue;
int arraysize;

outcount=0;

memset(temp,0,(MAX_SIZE*MAX_SIZE));		/* clear temporary array */

/* replace non-decimal numbers with decimal equivalents */

for(count=start;count<end;count++) { 
	if(memcmp(tokens[count],"0x",2) == 0) {	/* hex number */  
	    valptr=tokens[count];
	    valptr=valptr+2;

	    itoa(atoi_base(valptr,16),buf);
	    strcpy(tokens[count],buf);
	}

	if(memcmp(tokens[count],"0",1) == 0) {				/* octal number */
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

	if(strcmpi(tokens[count],"true") == 0 ) {	/* true */  
	    strcpy(tokens[count],"1");
	}

	if(strcmpi(tokens[count],"false") == 0 ) {	/* false */  
	    strcpy(tokens[count],"0");
	}

}

for(count=start;count<end;count++) {
	tokentype=0;

	if(CheckFunctionExists(tokens[count]) == 0) {	/* user function */
	 	tokentype=SUBST_FUNCTION;

	 	for(countx=count;countx<end;countx++) {		/* find end of function call */
			if(strcmp(tokens[countx],")") == 0) {
				countx++;
				break;
			}
	 	 }

		retval.has_returned_value=FALSE;

	  	if(CallFunction(tokens,count,countx) == -1) return(-1);

		if(retval.has_returned_value == TRUE) {		/* function has returned value */
		  	get_return_value(&subst_returnvalue);

		  	if(subst_returnvalue.type == VAR_STRING) {		/* returning string */   
		  		sprintf(temp[outcount++],"\"%s\"",retval.val.s);
		  	}
		  	else if(subst_returnvalue.type == VAR_INTEGER) {		/* returning integer */
				sprintf(temp[outcount++],"%d",retval.val.i);
	 	  	}
		  	else if(subst_returnvalue.type == VAR_NUMBER) {		/* returning double */
				sprintf(temp[outcount++],"%.6g",retval.val.d);		 		
		  	}
		  	else if(subst_returnvalue.type == VAR_SINGLE) {		/* returning single */
				sprintf(temp[outcount++],"%f",retval.val.f);
		  	}
		 	else if(subst_returnvalue.type == VAR_LONG) {			/* returning long */
				sprintf(temp[outcount++],"%ld",retval.val.l);
		  	}
	    }

		  	count=countx+1;
	 }
	 else if(IsVariable(tokens[count]) == TRUE) {
	    skiptokens=ParseVariableName(tokens,count,end,&split);	/* split variable name */
	    if(skiptokens == -1) return(-1);

	    tokentype=SUBST_VAR;

	    arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));

	    if(GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy) == -1) return(-1);

	    if(split.arraytype == ARRAY_SUBSCRIPT) {
		    if(((split.x*split.y) > arraysize) || (arraysize < 0)) {
			SetLastError(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
			return(-1);
		    }
	    }
	    else if(split.arraytype == ARRAY_SLICE) {
		    if((split.x*split.y) > strlen(val.s)) {
		    	SetLastError(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
			return(-1);
		    }
	    }

	    type=GetVariableType(split.name); 	/* Get variable type */
	    if(type == VAR_UDT) {
		type=GetFieldTypeFromUserDefinedType(split.name,split.fieldname);		/* get field type id udt */
		if(type == -1) {
			SetLastError(TYPE_FIELD_DOES_NOT_EXIST);
			return(-1);
		}
	    }

	    if(type == VAR_STRING) {
		   if(split.arraytype == ARRAY_SLICE) {		/* part of string */
			if(GetVariableValue(split.name,NULL,0,0,&val,0,0) == -1) return(-1);

			b=val.s;			/* get start */
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
	    	      	arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));

	    		if(((split.x*split.y) > arraysize) || arraysize < 0) {
				SetLastError(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
				return(-1);
			}
	
		        if(GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy) == -1) return(-1);

		  	sprintf(temp[outcount++],"\"%s\"",val.s);

		      }
	      
	            }


		}
		else if(type == VAR_NUMBER) {
		    sprintf(temp[outcount++],"%.6g",val.d);
		}
		else if(type == VAR_INTEGER) {
		    sprintf(temp[outcount++],"%d",val.i);
		}
		else if(type == VAR_SINGLE) {   
		    sprintf(temp[outcount++],"%f",val.f);

	       	}
		else if(type == VAR_LONG) {   
		    sprintf(temp[outcount++],"%ld",val.l);

	       	}
	 }
	else if (((char) *tokens[count] == '"') || IsSeperator(tokens[count],TokenCharacters) || IsNumber(tokens[count])) {
		strcpy(temp[outcount++],tokens[count]);
	}   
	else
	{
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	count += skiptokens;

//	if(count >= end) break;
}

/* copy tokens */

memset(out,0,MAX_SIZE*MAX_SIZE);

for(count=0;count<outcount;count++) {
	strcpy(out[count],temp[count]);
}

return(outcount);
}

/*
 *  Conatecate strings
 * 
 *  In: start	Start of variables in tokens array
	end	End of variables in tokens array
	tokens 	Tokens array
	val 	Variable value to return conatecated strings
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */
int ConatecateStrings(int start,int end,char *tokens[][MAX_SIZE],varval *val) {
int count;
char *sourceptr;
char *destptr;
char *temp[MAX_SIZE];
int size=0;

/* get size of output buffer */

for(count=start;count<end;count++) {
	size += strlen(tokens[count]);
}

val->type=VAR_STRING;
val->s=malloc(size);			/* allocate output buffer */

destptr=val->s;				/* point to output buffer */

*destptr++='"';		/* put " at start */

for(count=start;count<end;count++) {
	if(strcmpi(tokens[count],"+") == 0) { 

		/* not a string literal or string variable */
		if(GetVariableType(tokens[count+1]) != VAR_STRING) {
		   	SetLastError(TYPE_ERROR);
			return(-1);
		}
	}
	else
	{
		StripQuotesFromString(tokens[count],temp);	/* remove quotes from string */
		strcat(val->s,temp);
	}
}

strcat(val->s,"\"");		/* put " at end */

SetLastError(0);
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

	if(vartypenames[typecount] == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);		/* invalid type */
		return(-1);
	}

return(typecount);
}

/*
 *  Check if variable
 * 
 *  In: varname		Variable name
 * 
 *  Returns -1 on failure or 0 on success
 * 
 */
int IsVariable(char *varname) {
vars_t *next;
varsplit split;
int tc;

if(currentfunction == NULL) return(-1);

next=currentfunction->vars;						/* point to variables */

while(next != NULL) {
	if(strcmpi(next->varname,varname) == 0) return(TRUE);		/* variable exists */
	
	next=next->next;
}

return(FALSE);
}

/*
 *  Check if function
 * 
 *  In: funcname	Function name
 * 
 *  Returns TRUE or FALSE
 * 
 */
int IsFunction(char *funcname) {
functions *next=funcs;

while(next != NULL) {
	if(strcmpi(next->name,funcname) == 0) return(TRUE);
	
	next=next->next;
}

return(FALSE);
}

/*
 *  Push entry onto function call stack
 * 
 *  In: func		Function call stack entry
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int PushFunctionCallInformation(FUNCTIONCALLSTACK *func) {
FUNCTIONCALLSTACK *NewTop;

if(functioncallstack == NULL) {
	functioncallstack=malloc(sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */
	if(functioncallstack == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
	
	FunctionCallStackTop=functioncallstack;
}
else	
{
	NewTop=malloc(sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */
	if(NewTop == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	NewTop->next=FunctionCallStackTop;			/* push onto top of stack */
	FunctionCallStackTop=NewTop;
}

strcpy(FunctionCallStackTop->name,func->name);		/* copy information */
FunctionCallStackTop->callptr=func->callptr;
FunctionCallStackTop->startlinenumber=func->startlinenumber;
FunctionCallStackTop->currentlinenumber=func->startlinenumber;
FunctionCallStackTop->saveinformation=func->saveinformation;
FunctionCallStackTop->saveinformation_top=func->saveinformation_top;
FunctionCallStackTop->vars=func->vars;
FunctionCallStackTop->stat=func->stat;

if(GetFunctionPointer(func->name) != NULL) FunctionCallStackTop->parameters=GetFunctionPointer(func->name)->parameters;

FunctionCallStackTop->moduleptr=func->moduleptr;
strcpy(FunctionCallStackTop->returntype,func->returntype);
FunctionCallStackTop->type_int=func->type_int;
FunctionCallStackTop->lastlooptype=func->lastlooptype;

currentfunction=FunctionCallStackTop;

SetLastError(0);
return(0);
}

/*
 *  Pop entry from function call stack
 * 
 *  In: Nothing
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int PopFunctionCallInformation(void) {
vars_t *vars;
FUNCTIONCALLSTACK *OldTop;

if(currentfunction == NULL) return(-1);

/* remove variables */

vars=currentfunction->vars;

while(vars != NULL) {
	 RemoveVariable(vars->varname);
	 vars=vars->next;
}

/* remove entry from function call stack */

if(FunctionCallStackTop != NULL) {
	OldTop=FunctionCallStackTop;				/* save current top of stack */

	FunctionCallStackTop=FunctionCallStackTop->next;	/* remove from top of stack */

	free(OldTop);						/* free old top of stack */

	currentfunction=FunctionCallStackTop;			/* set current function */
}

SetCurrentBufferPosition(currentfunction->callptr);

return(0);
}

/*
 *  Find first variable for current function
 * 
 *  In: var	Buffer to hold variable information
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int FindFirstVariable(vars_t *var) {
if(currentfunction->vars == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

findvar=currentfunction->vars;

memcpy(var,currentfunction->vars,sizeof(vars_t));

findvar=findvar->next;
}

/*
 *  Find next variable for current function
 * 
 *  In: var	Buffer to hold variable information
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int FindNextVariable(vars_t *var) {
if(findvar == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

memcpy(var,findvar,sizeof(vars_t));

findvar=findvar->next;
}


/*
 *  Find named variable for current function
 * 
 *  In: name	Variable name
 *	var	Buffer to hold variable information
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */

int FindVariable(char *name,vars_t *var) {
vars_t *next;

next=currentfunction->vars;

while(next != NULL) {
	if(strcmp(next->varname,name) == 0) {
		memcpy(var,next,sizeof(vars_t));

		SetLastError(0);
		return(0);
	}

	next=next->next;
}


SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Get pointer to user-defined type entry
 * 
 *  In: name	Name of user-defined type
 * 
 *  Returns: Pointer to entry on success or NULL on failure
 * 
 */
UserDefinedType *GetUDT(char *name) {
	UserDefinedType *next=udt;

	while(next != NULL) {
		if(strcmpi(next->name,name) == 0) return(next);	/* found UDT */

		next=next->next;
	}

	return(NULL);
}

/*
 *  Add user-defined type
 * 
 *  In: newudt	New user-defined type
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int AddUserDefinedType(UserDefinedType *newudt) {
UserDefinedType *next;
UserDefinedType *last;
UserDefinedTypeField *fieldptr;

if(udt == NULL) {			/* first entry */
	udt=malloc(sizeof(UserDefinedType));		/* add link to the end */
	if(udt == NULL) {
		SetLastError(NO_MEM); 
		return(-1);
	}

	last=udt;
}
else
{
/* find end and check if UDT exists */

	next=udt;

	while(next != NULL) {
 		last=next;

 		if(strcmpi(next->name,newudt->name) == 0) {	/* udt exists */
			SetLastError(TYPE_EXISTS); 
			return(-1);
		}

		next=next->next;
	}

	/* add link to the end */	

	last->next=malloc(sizeof(UserDefinedType));
	if(last->next == NULL) {
		SetLastError(NO_MEM); 
		return(-1);
	}

	last=last->next;
}

memcpy(last,newudt,sizeof(UserDefinedType));

SetLastError(0);
return(0);
}

/*
 *  Copy user-defined type entry
 * 
 *  In: source	User-defined type entry to copy
 *	dest	User-defined type entry to copy to
 *
 *  Returns: 0 on success, -1 on failure
 * 
 */
int CopyUDT(UserDefinedType *source,UserDefinedType *dest) {
UserDefinedTypeField *sourcenext;
UserDefinedTypeField *destnext;
int count;

dest->field=malloc(sizeof(UserDefinedTypeField));
if(dest->field == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

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
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(double))*(sourcenext->ysize*sizeof(double)));
	}
	else if(sourcenext->type == VAR_SINGLE) {		/* single precision */			
		destnext->fieldval=malloc((sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
	}
	if(sourcenext->type == VAR_INTEGER) {		/* integer */
	destnext->fieldval=malloc((sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
	}
	if(sourcenext->type == VAR_LONG) {		/* long */
	destnext->fieldval=malloc((sourcenext->xsize*sizeof(long long))*(sourcenext->ysize*sizeof(long long)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(long long))*(sourcenext->ysize*sizeof(long long)));
	}
	else if(sourcenext->type == VAR_STRING) {	/* string */			
		destnext->fieldval=calloc(((sourcenext->xsize*MAX_SIZE)*(sourcenext->ysize*MAX_SIZE)),sizeof(char *));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

	/* copy strings in array */

		for(count=0;count<(sourcenext->xsize*sourcenext->ysize);count++) {
	 		if(sourcenext->fieldval->s[count] == NULL) {
				sourcenext->fieldval[count].s=malloc(strlen(destnext->fieldval[count].s));
				if(sourcenext->fieldval[count].s == NULL) {
					SetLastError(NO_MEM);
					return(-1);
				}

				strcpy(destnext->fieldval[count].s,sourcenext->fieldval->s[count]);
			}
		}
	 }

	 destnext->next=malloc(sizeof(UserDefinedTypeField));
	 if(dest->next == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	 }

	}

SetLastError(0);
return(0);
}


/*
 *  Get pointer to variable list entry 
 * 
 *  In: name	Name of variable
 * 
 *  Returns: Pointer to variable on success, NULL on failure
 * 
 */
vars_t *GetVariablePointer(char *name) {
vars_t *next;

if(currentfunction == NULL) return(NULL);

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
 *  Returns: 0 in success or -1 on failure
 * 
  */

int GetFieldValueFromUserDefinedType(char *varname,char *fieldname,varval *val,int fieldx,int fieldy) {
UserDefinedTypeField *udtfield;
vars_t *next;

next=GetVariablePointer(varname);		/* get variable entry */
if(next == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

udtfield=next->udt->field;

while(udtfield != NULL) {

	if(strcmp(udtfield->fieldname,fieldname) == 0) {	/* found field */				
		val->type=udtfield->type;
		if(udtfield->type == VAR_NUMBER) {
		        val->d=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].d;

		        SetLastError(0);
			return(0);
		 }
		 else if(udtfield->type == VAR_STRING) {
			val->s=malloc(strlen(udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s)+1);	/* allocate string */
			if(val->s == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

		        strcpy(val->s,udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s);

		        SetLastError(0);
			return(0);
		 }
		 else if(udtfield->type == VAR_INTEGER) {
		        val->i=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].i;
			SetLastError(0);
		 }
		 else if(udtfield->type == VAR_SINGLE) {
		        val->f=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].f;
		        SetLastError(0);
			return(0);
		 }
		 else {
		        SetLastError(0);
			return(0);
		}	
	 }

	udtfield=udtfield->next;
	}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}	

/*
*  Check if valid variable type
* 
*  In: type	variable type name
* 
*  Returns variable type, -1 otherwise
* 
*/

int IsValidVariableType(char *type) {
int count=0;

while(vartypenames[count] != NULL) {

	if(strcmpi(vartypenames[count],type) == 0) {
		SetLastError(NO_ERROR);
		return(count);
	}

	count++;
}

SetLastError(INVALID_VARIABLE_TYPE);
return(-1);
}

/*
 *  Get field type from user defined type field
 * 
 *  In: varname		Variable
 *  	fieldname	Name of the UDT field to return value of
 * 
 *  	val		Value buffer
 *  	x		x subscript
 *  	y		y subscript
 * 
 *  Returns: field type on success or -1 on failure
 * 
  */

int GetFieldTypeFromUserDefinedType(char *varname,char *fieldname) {
UserDefinedTypeField *udtfield;
vars_t *next;

next=GetVariablePointer(varname);		/* get variable entry */
if(next == NULL) {
	SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
	return(-1);
}

udtfield=next->udt->field;

while(udtfield != NULL) {
	if(strcmpi(udtfield->fieldname,fieldname) == 0) return(udtfield->type);

	udtfield=udtfield->next;
}

SetLastError(TYPE_DOES_NOT_EXIST);
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
if(currentfunction == NULL) return;

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
if(currentfunction == NULL) return(-1);

return(currentfunction->currentlinenumber);
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
if(currentfunction == NULL) return(-1);

currentfunction->currentlinenumber=linenumber;
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
if(currentfunction == NULL) return;

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
if(currentfunction == NULL) return;

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
if(currentfunction == NULL) return;

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
if(currentfunction == NULL) return(-1);

return(currentfunction->stat);
}

/*
*  Get save information buffer pointer
*
*  In: Nothing
* 
*  Returns: buffer pointer or NULL.
* 
*/
char *GetSaveInformationBufferPointer(void) {
if(currentfunction->saveinformation_top->bufptr == NULL) return(NULL);

return(currentfunction->saveinformation_top->bufptr);
}

/*
*  Set save information buffer pointer
*
*  In: buffer pointer
* 
*  Returns: nothing
* 
*/
void SetSaveInformationBufferPointer(char *ptr) {
if(currentfunction == NULL) return;

currentfunction->saveinformation_top->bufptr=ptr;
}

/*
*  Get save information buffer pointer
*
*  In: Nothing
* 
*  Returns: buffer pointer or NULL on error
* 
*/
int GetSaveInformationLineCount(void) {
SAVEINFORMATION *info;

if(currentfunction == NULL) return(-1);

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
if(currentfunction == NULL) return(-1);

return(currentfunction->type_int);
}

/*
 *  Push internal save state information to save information stack
 * 
 *  In: Nothing
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int PushSaveInformation(void) {
SAVEINFORMATION *info;
SAVEINFORMATION *newinfo;

if(currentfunction == NULL) return(-1);

if(currentfunction->saveinformation_top == NULL) {
	currentfunction->saveinformation_top=malloc(sizeof(SAVEINFORMATION));		/* allocate new entry */

	if(currentfunction->saveinformation_top == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
}
else
{
	newinfo=malloc(sizeof(SAVEINFORMATION));		/* allocate new entry */
	if(newinfo == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	newinfo->next=currentfunction->saveinformation_top;	/* put entry at top of stack */
	currentfunction->saveinformation_top=newinfo;
}

currentfunction->saveinformation_top->bufptr=GetCurrentBufferPosition();
currentfunction->saveinformation_top->linenumber=GetCurrentFunctionLine();

return(0);
}

/*
 *  Pop internal save state information from save information stack
 * 
 *  In: Nothing
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */

int PopSaveInformation(void) {
SAVEINFORMATION *oldtop;

if(currentfunction == NULL) return(-1);

oldtop=currentfunction->saveinformation_top;		/* get current topmost entry */

currentfunction->saveinformation_top=currentfunction->saveinformation_top->next;	/* remove from stack */

free(oldtop);		/* free entry */

if(currentfunction->saveinformation_top != NULL) {
	SetCurrentBufferPosition(currentfunction->saveinformation_top->bufptr);
	currentfunction->startlinenumber=currentfunction->saveinformation_top->linenumber;
}

return(0);
}

/*
 *  Get pointer to top of save information stack
 * 
 *  In: Nothing
 * 
 *  Returns: pointer to top of save information stack on success or NULL on failure
 * 
 */

SAVEINFORMATION *GetSaveInformation(void) {
if(currentfunction == NULL) return(NULL);

return(currentfunction->saveinformation_top);
}

/*
*  Get variable X size
* 
*  In: Nothing
* 
*  Returns: return variable X size or -1 on error
* 
*/

int GetVariableXSize(char *name) {
vars_t *next;

if(currentfunction == NULL) return(-1);

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) return(next->xsize);
	 
	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
*  Get variable Y size
* 
*  In: Nothing
* 
*  Returns: return variable Y size or -1 on error
* 
*/

int GetVariableYSize(char *name) {
vars_t *next;

if(currentfunction == NULL) return(-1);

next=currentfunction->vars;

while(next != NULL) {
	if(strcmpi(next->varname,name) == 0) return(next->ysize);
	 
	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
*  Get function call stack top
* 
*  In: Nothing
* 
*  Returns: pointer to top of call stack
* 
*/

FUNCTIONCALLSTACK *GetFunctionCallStackTop(void) {
return(FunctionCallStackTop);
}	

/*
*  Check if variable or keyword is valid
* 
*  In: variable name
* 
*  Returns: TRUE or FALSE
*
*/

int IsValidVariableOrKeyword(char *name) {
char *InvalidChars = { "¬`\"$%^&*()-+={}[]:;@'~#<>,.?/|\\" };

if(strpbrk(name,InvalidChars) != NULL) return(FALSE);		/* can't start with invalid character */

return(TRUE);
}

/*
*  Free variables lists
* 
*  In: vars	Pointer to variables list
* 
*  Returns: Nothing
*
*/

void FreeVariablesList(vars_t *vars) {
vars_t *next=vars;
int count;

while(next != NULL) {
	/* If it's a string variable, free it */
	if(next->type_int == VAR_STRING) {
		for(count=0;count<(next->xsize*next->ysize);count++) {
			free(next->val[count].s);
		}
	}


//	free(next);

	next=next->next;
}

return;
}

/*
*  Free functions and variables lists
* 
*  In: Nothing
* 
*  Returns: Nothing
*
*/
void FreeFunctionsAndVariables(void) {
functions *funcnext=funcs;
functions *funcptr;
vars_t *varnext;
FUNCTIONCALLSTACK *callstacknext=functioncallstack;
FUNCTIONCALLSTACK *callstackptr;
SAVEINFORMATION *savenext;

/* loop through functions and free them
   There is no need to set the next and last entries in the lists to NULL, because the entire list will be deallocated.
 */

while(funcnext != NULL) {
	FreeVariablesList(funcnext->parameters);	/* free parameters */

	if(funcnext->WasDeclaredInInteractiveMode == TRUE) free(funcnext->funcstart);	/* function was declared in interactive mode */

	funcptr=funcnext->next;

	free(funcnext);
	funcnext=funcptr;
}

funcs=NULL;

/* loop through and release call stack */

while(callstacknext != NULL) {
	FreeVariablesList(callstacknext->vars);		/* free variables */

	savenext=callstacknext->saveinformation;

	while(savenext != NULL) {
		free(savenext);

		savenext=savenext->next;
	}
	
	callstackptr=callstacknext->next;
	free(callstacknext);
	callstacknext=callstackptr;
}

functioncallstack=NULL;
return;
}

/*
*  Determine if string is numeric or not
* 
*  In: string
* 
*  Returns: TRUE or FALSE
*
*/
int IsNumber(char *token) {
char *tokenptr=token;

while((char) *tokenptr != 0) {
	if( ((char ) *tokenptr < '0') || ((char ) *tokenptr > '9')) return(FALSE);
	
	tokenptr++;
}

return(TRUE);
}

/*
*  Get pointer to function list entry
* 
*  In: Function name
*
*  Returns: pointer to function list entry
*
*/
functions *GetFunctionPointer(char *name) {
functions *funcnext=funcs;

while(funcnext != NULL) {
	if(strcmp(funcnext->name,name) == 0) return(funcnext);	/* found name */

	funcnext=funcnext->next;
}

return(NULL);
}

