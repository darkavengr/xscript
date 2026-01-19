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
#include "version.h"
#include "statements.h"

extern char *TokenCharacters;

functions *funcs=NULL;
functions *funcs_end=NULL;
FUNCTIONCALLSTACK *functioncallstack=NULL;
FUNCTIONCALLSTACK *FunctionCallStackTop=NULL;
FUNCTIONCALLSTACK *currentfunction=NULL;
UserDefinedType *udt=NULL;
char *BuiltInVariableTypes[] = { "DOUBLE","STRING","INTEGER","SINGLE","LONG","BOOLEAN",NULL };
char *truefalse[] = { "False","True" };
functionreturnvalue retval;
vars_t *findvar;
int callpos=0;

/*
 * Intialize main function
 * 
 * In: 	progname	Script filename
 *	args		command-line arguments
 * 
 *  Returns: 0 on success, -1 on failure
 * 
 */
int InitializeMainFunction(char *progname,char *args) {
FUNCTIONCALLSTACK newfunc;
functions mainfunc;
char *tokens[MAX_SIZE][MAX_SIZE];
int NumberOfDeclareTokens;
int whichreturntype;

/* Add main function to list of functions */

NumberOfDeclareTokens=0;

strncpy(tokens[0],"main",MAX_SIZE);
strncpy(tokens[1],"(",MAX_SIZE);
strncpy(tokens[2],")",MAX_SIZE);
strncpy(tokens[3],"as",MAX_SIZE);
strncpy(tokens[4],"integer",MAX_SIZE);

if(DeclareFunction(tokens,5) == -1) return(-1);			/* declare main function */

/* push main function onto call stack */
strncpy(newfunc.name,"main",MAX_SIZE);

newfunc.callptr=NULL;
newfunc.startlinenumber=1;
newfunc.currentlinenumber=1;
newfunc.saveinformation_top=NULL;
newfunc.initialparameters=NULL;
newfunc.vars=NULL;
newfunc.vars_end=NULL;
newfunc.stat=0;
newfunc.moduleptr=GetCurrentModuleInformationFromBufferAddress(GetCurrentBufferAddress());

strncpy(newfunc.returntype,BuiltInVariableTypes[VAR_INTEGER],MAX_SIZE);

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

cmdargs.s=calloc(1,MAX_SIZE);
if(cmdargs.s == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

/* add built-in variables */

cmdargs.i=0;

CreateVariable("ERR","INTEGER",1,1);			/* error number */
CreateVariable("ERRL","INTEGER",1,1);			/* error line */
CreateVariable("ERRFUNC","STRING",1,1);			/* error function */

if(progname != NULL) {
	CreateVariable("PROGRAMNAME","STRING",1,1);		/* script name */

	strncpy(cmdargs.s,progname,MAX_SIZE);
	UpdateVariable("PROGRAMNAME","",&cmdargs,0,0,0,0);
}


/* Command-line arguments, if any */

if(args != NULL) {
	CreateVariable("COMMAND","STRING",1,1);

	strncpy(cmdargs.s,args,MAX_SIZE);
	UpdateVariable("COMMAND","",&cmdargs,0,0,0,0);
}

/* XScript version */
CreateVariable("VERSION","DOUBLE",1,1);

cmdargs.d=(double) XSCRIPT_VERSION_MAJOR+((double) XSCRIPT_VERSION_MINOR/100)+((double) XSCRIPT_VERSION_REVISION/1000);
UpdateVariable("VERSION","",&cmdargs,0,0,0,0);

GetVariablePointer("VERSION")->IsConstant=TRUE;		/* set variable as constant */
free(cmdargs.s);
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

//printf("CREATE name=%s (%d,%d)\n",name,xsize,ysize);

if((xsize == 0) || (ysize == 0)) {		/* invalid array size */
	SetLastError(INVALID_ARRAY_SIZE);
	return(-1);
}

if(currentfunction == NULL) return(-1);

if(IsStatement(name)) {				/* Check if variable name is a reserved name */
	SetLastError(INVALID_VARIABLE_NAME);
	return(-1);
}

if(IsVariable(name)) {				/* check if variable exists */
	SetLastError(VARIABLE_EXISTS);
	return(-1);
}

/* Add entry to variable list */

if(currentfunction->vars == NULL) {			/* first entry */
	currentfunction->vars=calloc(1,sizeof(struct vartype));		/* add new item to list */
	if(currentfunction->vars == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end=currentfunction->vars;
}
else
{
	currentfunction->vars_end->next=calloc(1,sizeof(struct vartype));		/* add new item to list */
	if(currentfunction->vars_end->next == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end=currentfunction->vars_end->next;
}

/* add to end */

count=IsBuiltInVariableType(type);		/* get built-in variable type */

if(count != -1) {				/* is built-in variable type */
	currentfunction->vars_end->val=calloc(1,xsize*ysize*sizeof(varval));
	if(currentfunction->vars_end->val == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	currentfunction->vars_end->type_int=count;
}
else
{
	currentfunction->vars_end->type_int=VAR_UDT;

	strncpy(currentfunction->vars_end->udt_type,type,MAX_SIZE);		/* set udt type */

	usertype=GetUDT(type);
	if(usertype == NULL) {
		SetLastError(INVALID_VARIABLE_TYPE);
		return(-1);
	}

	currentfunction->vars_end->udt=calloc(1,(xsize*ysize)*sizeof(UserDefinedType));	/* allocate user-defined type */
	if(currentfunction->vars_end->udt == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	strncpy(currentfunction->vars_end->udt,type,MAX_SIZE);		/* Copy type */

	currentfunction->vars_end->udt->field=calloc(1,sizeof(UserDefinedTypeField));
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

			strncpy(fieldptr->fieldname,udtfieldptr->fieldname,MAX_SIZE);
			fieldptr->type=udtfieldptr->type;
		
			fieldptr->fieldval=calloc(1,sizeof(struct vartype));
			if(fieldptr->fieldval == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

			fieldptr->next=calloc(1,sizeof(UserDefinedTypeField));		/* allocate field in user-defined type */
			if(fieldptr->next == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

			fieldptr=fieldptr->next;			
			udtfieldptr=udtfieldptr->next;
		}			
	}
}

currentfunction->vars_end->xsize=xsize;				/* set size */
currentfunction->vars_end->ysize=ysize;

strncpy(currentfunction->vars_end->varname,name,MAX_SIZE);		/* set name */
currentfunction->vars_end->next=NULL;

//SetLastError(NO_ERROR);
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
vars_t *next=NULL;
char *o;
varval *varv;
char *strptr;
varsplit split;
UserDefinedType *udtptr;
UserDefinedTypeField *fieldptr;
UserDefinedTypeField *udtfield;

//printf("UPDATE name=%s (%d,%d)\n",name,x,y);

if(currentfunction == NULL) {
	return(-1);
}
else
{
	next=GetVariablePointer(name);		/* get variable entry */
	if(next == NULL) {
		PrintError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}
}

if( ((x*y) > (next->xsize*next->ysize)) || ((x*y) < 0)) {		/* outside array */
	PrintError(INVALID_ARRAY_SUBSCRIPT);
	return(-1);
}

if(next->IsConstant == TRUE) {			/* if constant variable */
	printf("SET CONSTANT=%d\n",IS_CONSTANT);

	SetLastError(IS_CONSTANT);
	return(-1);
}

/* update variable */

if(next->type_int == VAR_NUMBER) {		/* double precision */
	next->val[x*y].d=val->d;

	return(0);
}
else if(next->type_int == VAR_STRING) {	/* string */
	if(next->val[x*y].s == NULL) {		/* if string element not allocated */
		next->val[x*y].s=calloc(1,strlen(val->s)+1);	/* allocate memory */
		if(next->val[x*y].s == NULL) {
			PrintError(NO_MEM);
			return(-1);
		}
	} 

	strncpy(next->val[x*y].s,val->s,strlen(val->s));	/* copy value */
	return(0);
}
else if(next->type_int == VAR_INTEGER) {	/* integer */
	next->val[x*y].i=val->i;
	return(0);
}
else if(next->type_int == VAR_SINGLE) {	/* single */
	next->val[x*y].f=val->f;
	return(0);
}	
else if(next->type_int == VAR_LONG) {	/* long */
	next->val[x*y].l=val->l;
	return(0);
}
else if(next->type_int == VAR_BOOLEAN) {	/* boolean */
	next->val[x*y].b=val->b;
	return(0);
}
else {					/* user-defined type */	
	udtfield=next->udt->field;

	while(udtfield != NULL) {
		if(strcmp(udtfield->fieldname,fieldname) == 0) {	/* found field */
			val->type=udtfield->type;

			if(udtfield->type == VAR_NUMBER) {
			        udtfield->fieldval[x*y].d=val->d;

				return(0);
			 }
			 else if(udtfield->type == VAR_STRING) {
			 	if(udtfield->fieldval[x*y].s == NULL) {		/* if string element not allocated */
	 				udtfield->fieldval[x*y].s=calloc(1,strlen(val->s));	/* allocate memory */
					if(udtfield->fieldval[x*y].s == NULL) {
						SetLastError(NO_MEM);
						return(-1);
					}
	      			} 

	      			if( strlen(val->s) > (strlen(udtfield->fieldval[x*y].s)+x)) {	/* if string element larger */
	 				realloc(udtfield->fieldval[x*y].s,strlen(val->s));	/* resize memory */
	      			}
		
			        strncpy(udtfield->fieldval[x*y].s,val->s,strlen(udtfield->fieldval[y*y].s));		/* assign value */
				return(0);
			 }
			 else if(udtfield->type == VAR_INTEGER) {
			        udtfield->fieldval[x*y].i=val->i;
				return(0);
			 }
			 else if(udtfield->type == VAR_SINGLE) {
			        udtfield->fieldval[x*y].f=val->f;
				return(0);
			 }
			 else {
				PrintError(INVALID_VARIABLE_TYPE);
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

if(currentfunction == NULL) {
	return(-1);
}
else
{
	next=GetVariablePointer(name);		/* get variable entry */
	if(next == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}
}

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

/*
 *  Get variable value
 * 
 *  In: name	Variable name
	val	Variable value
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy) {
vars_t *next;
UserDefinedTypeField *udtfield;

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

	      case VAR_BOOLEAN:
		if(atoi(name) > 1) {		/* Invalid boolean value */
			SetLastError(TYPE_ERROR);
			return(-1);
		}

	      default:
	      	val->d=atof(name);
		break;
	   }

	SetLastError(0);
}

if((char) *name == '"') {
	val->s=calloc(1,strlen(name)+2);				/* allocate string */
	if(val->s == NULL) SetLastError(NO_MEM);

	strncpy(val->s,name,strlen(name));
	SetLastError(0);
}

if(currentfunction == NULL) {
	return(-1);
}
else
{
	next=GetVariablePointer(name);		/* get variable entry */
	if(next == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}
}

if( (x > next->xsize) || (y > next->ysize)) {
	SetLastError(INVALID_ARRAY_SUBSCRIPT);	/* outside array */
	return(-1);
}

if(next->type_int == VAR_NUMBER) {
	val->d=next->val[x*y].d;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_STRING) {
	val->s=calloc(1,strlen(next->val[x*y].s)+2);				/* allocate string */
	if(val->s == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	strncpy(val->s,next->val[x*y].s,strlen(next->val[x*y].s));

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_INTEGER) {
	val->i=next->val[x*y].i;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_SINGLE) {
	val->f=next->val[x*y].f;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_LONG) {
	val->l=next->val[x*y].l;

	SetLastError(0);
	return(0);
}
else if(next->type_int == VAR_BOOLEAN) {	/* boolean */
	val->b=next->val[x*y].b;
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

if(currentfunction == NULL) {
	return(-1);
}
else
{
	next=GetVariablePointer(name);		/* get variable entry */
	if(next == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}
}	

return(next->type_int);		/* return variable type */
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
int commacount;
char ParseEndChar;
char *evaltokens[MAX_SIZE][MAX_SIZE];
int evaltc;
int varend;
int tokencount=0;

memset(split,0,sizeof(varsplit));

strncpy(split->name,tokens[start],MAX_SIZE);		/* copy name */

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
	commacount=0;

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

			if(strcmp(tokens[count],",") == 0) {
				if((strcmp(tokens[count-1],"(") == 0) || (strcmp(tokens[count+1],")") == 0)) {	/* no expression before or after
														   comma found */
					SetLastError(SYNTAX_ERROR);
					return(-1);
				}

				if(commacount == 0) subscriptstart=count;	/* save comma position */

				commacount++;		 /* comma found */
			}
	}

	if(commacount > 1) {			/* invalid number of commas */
		SetLastError(SYNTAX_ERROR);
		return(-1);
	}
	else if(commacount == 0) {			/* 2d array */
		if(IsValidExpression(tokens,subscriptstart+1,subscriptend-1) == FALSE) return(-1);	/* invalid expression */

		evaltc=SubstituteVariables(subscriptstart,subscriptend,tokens,evaltokens);

		split->x=EvaluateExpression(evaltokens,0,evaltc);
	 	split->y=1;
	}
	else
	{
		/* invalid expression */
		if((IsValidExpression(tokens,subscriptstart+1,count-1) == FALSE) || (IsValidExpression(tokens,count+1,subscriptend-1) == FALSE)) {
			SetLastError(INVALID_EXPRESSION);
			return(-1);
		}
	
		evaltc=SubstituteVariables(subscriptstart,count,tokens,evaltokens);
		split->x=EvaluateExpression(evaltokens,0,evaltc);

		evaltc=SubstituteVariables(count+1,subscriptend,tokens,evaltokens);
		split->y=EvaluateExpression(evaltokens,0,evaltc);
	}

	
}

if(fieldstart != start) {					/* if there is a field name and possible subscripts */
	strncpy(split->fieldname,tokens[fieldstart],MAX_SIZE);	/* copy field name */
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
 *  Free variable values
 * 
 *  In: val	variable value
	type	variable type
	xsize	X size
	ysize	Y size
 * 
 *  Returns: Nothing
 * 
  */
void FreeVariableValues(varval *val,int type,int xsize,int ysize) {
int count;

if(type == VAR_STRING) {	/* freeing string variable */
	for(count=0;count < (xsize*ysize);count++) {
		if(val[count].s != NULL) free(val[count].s);	/* free string element */
	}
}

if(val != NULL) free(val);		/* free entry */
}

/*
 *  Remove variable
 * 
 *  In: name	Variable name
 * 
 *  Returns -1 on failure or 0 on success
 * 
  */
int RemoveVariable(char *name) {
vars_t *next;
vars_t *prev;
vars_t *save;
int count;
UserDefinedTypeField *fieldnext;
UserDefinedTypeField *fieldprev;
UserDefinedTypeField *fieldptr;
UserDefinedTypeField *fieldsave;

next=currentfunction->vars;						/* point to variables */
prev=currentfunction->vars;

while(next != NULL) {
	
	if(strcmpi(next->varname,name) == 0) {			/* found variable */
		if(next->type_int == VAR_UDT) {		/* freeing UDT variable */
			/* remove UDT fields */

			fieldptr=next->udt->field;
			prev=fieldptr;

			while(fieldptr != NULL) {
				FreeVariableValues(fieldptr->fieldval,fieldptr->type,fieldptr->xsize,fieldptr->ysize);	/* remove values */

				if(fieldptr == next->udt->field) {		/* first */
					fieldsave=next->udt->field;

					next->udt->field=next->udt->field->next;
			
					free(fieldsave);
				}
				else if(fieldnext->next == NULL) {			/* last entry */
					fieldsave=fieldprev->next;
					fieldprev->next=NULL;

					free(fieldsave);
				}
				else						/* middle entry */
				{
  					prev->next=fieldnext->next;
					free(fieldnext);
				} 

				fieldprev=fieldnext;
				fieldnext=fieldnext->next;
			}
		}
		else					/* other variable type */
		{
				FreeVariableValues(next->val,next->type_int,next->xsize,next->ysize);		/* remove values */
		}
	
		/* remove the entry from the variables list */

		if(next == currentfunction->vars) {		/* first */
			save=currentfunction->vars;

			currentfunction->vars=currentfunction->vars->next;
			free(save);

			SetLastError(0);
			return(0);  

		}
		else if (next->next == NULL) {			/* last entry */
			save=prev->next;
			prev->next=NULL;

			free(save);

			SetLastError(0);
			return(0); 
		}
		else						/* middle entry */
		{
	  		prev->next=next->next;
			free(next);

			SetLastError(0);
			return(0);  
		} 
	}

	prev=next;
 	next=next->next;
}

SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
return(-1);
}

/*
 *  Declare function
 * 
 *  In: tokens[MAX_SIZE][MAX_SIZE] function name and args
	funcargcount		   number of tokens
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
int vartoken;
bool ParameterIsConstant=FALSE;
int paramtype;

if((currentfunction != NULL) && ((currentfunction->stat & FUNCTION_STATEMENT) == FUNCTION_STATEMENT)) {
	SetLastError(NESTED_FUNCTION);
	return(-1);
}

if(IsFunction(tokens[0]) == TRUE) {	/* Check if function already exists */
	SetLastError(FUNCTION_EXISTS);
	return(-1);
}

if(funcs == NULL) {
	funcs=calloc(1,sizeof(struct func));		/* add new item to list */

	if(funcs == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	funcs_end=funcs;	
	funcs_end->last=NULL;
}
else
{
	funcs_end->next=calloc(1,sizeof(struct func));		/* add new item to list */
	if(funcs_end->next == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	previous=funcs_end;
	funcs_end=funcs_end->next;

	funcs_end->last=previous;		/* point to previous entry */
}

strncpy(funcs_end->name,tokens[0],MAX_SIZE);				/* copy name */

funcs_end->funcstart=GetCurrentBufferPosition();
if(currentfunction == NULL) {
	funcs_end->linenumber=1;
}
else
{
	funcs_end->linenumber=currentfunction->currentlinenumber;
}

/* skip ( and go to end */

count=2;

while(count < end) {
	ParameterIsConstant=FALSE;

	if(strcmpi(tokens[count],")") == 0) break;		/* function declaration without parameters */

	if(strcmpi(tokens[count+1], "AS") != 0) {		/* type */
		SetLastError(SYNTAX_ERROR);
		return(-1);
	}

	vartoken=count;

	if(strcmpi(tokens[count+2], "CONSTANT") == 0) {		/* constant variable declaration */
		ParameterIsConstant=TRUE;
		count++;		/* skip constant keyword */
	}

	/* check variable type */

	paramtype=IsBuiltInVariableType(tokens[count+2]);
	if(paramtype == -1) {		/* not built-in type */
		udtptr=GetUDT(tokens[count+2]);
		if(udtptr == NULL) {	/* not user-defined type */
			SetLastError(INVALID_VARIABLE_TYPE);
			return(-1);
		}

		strncpy(vartype,udtptr->name,MAX_SIZE);
	}
 	else
	{
		strncpy(vartype,tokens[count+2],MAX_SIZE);
	}

/* add parameter */

	if(funcs_end->parameters == NULL) {
		funcs_end->parameters=calloc(1,sizeof(struct vartype));
		funcs_end->parameters_end=funcs_end->parameters;
	}
	else
	{
		funcs_end->parameters_end->next=calloc(1,sizeof(struct vartype));		/* add to end */
	  	funcs_end->parameters_end=funcs_end->parameters_end->next;
	}

	/* add function parameters */

	if(BuiltInVariableTypes[paramtype] != NULL) {			/* built-in type */
		funcs_end->parameters_end->type_int=typecount;
	}
		else
	{
		funcs_end->parameters_end->type_int=VAR_UDT;
		strncpy(funcs_end->parameters_end->udt_type,tokens[count],MAX_SIZE);	/* user-defined type */
	}

	strncpy(funcs_end->parameters_end->varname,tokens[vartoken],MAX_SIZE);

	funcs_end->parameters_end->xsize=0;
	funcs_end->parameters_end->ysize=0;
	funcs_end->parameters_end->val=NULL;
	funcs_end->parameters_end->FunctionParameterIsConstant=ParameterIsConstant;	/* parameter is constant */
	NumberOfParameters++;

	if(strcmp(tokens[count+3],")") == 0) break;	

	count += 4;		/* skip "AS" and type */
}

count++;		/* point to AS type */

funcs_end->funcargcount=NumberOfParameters;

if(GetInteractiveModeFlag()) funcs_end->WasDeclaredInInteractiveMode=TRUE;	/* function was declared in interactive mode */

/* get function return type */

if(strcmpi(tokens[count], "AS") != 0) {
	SetLastError(SYNTAX_ERROR);
	return(-1);
}

vartoken=count;

if(strcmpi(tokens[count+1], "CONSTANT") == 0) count++;		/* skip CONSTANT keyword */

typecount=0;	

while(BuiltInVariableTypes[typecount] != NULL) {
	if(strcmpi(BuiltInVariableTypes[typecount],tokens[count+1]) == 0) break;

	typecount++;
}

if(BuiltInVariableTypes[typecount] == NULL) {
	udtptr=GetUDT(tokens[count+2]);
	if(udtptr == NULL) {	
		SetLastError(TYPE_DOES_NOT_EXIST);
		return(-1);
	}
}
else
{
	strncpy(funcs_end->returntype,BuiltInVariableTypes[typecount],MAX_SIZE);	
}

funcs_end->type_int=typecount;		/* copy function type */

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

	RemoveNewline(linebuf);		/* remove newline */

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
int varstc;
FUNCTIONCALLSTACK newfunc;
UserDefinedType userdefinedtype;
int returnvalue;
char *evaltokens[MAX_SIZE][MAX_SIZE];
int linenumber;
int NumberOfArguments=0;
varval paramval;
vars_t *parameters=NULL;
vars_t *initialparameters=NULL;
vars_t *var;

retval.has_returned_value=FALSE;			/* clear has returned value flag */

/* find function name */

next=funcs;						/* point to variables */
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

/* save information about the called function. The caller function is already on the stack */

currentfunction->callptr=GetCurrentBufferPosition();

/* save information about the called function */

strncpy(newfunc.name,next->name,MAX_SIZE);			/* function name */
newfunc.callptr=next->funcstart;			/* function start */

SetCurrentBufferPosition(next->funcstart);

newfunc.startlinenumber=next->linenumber;
newfunc.currentlinenumber=next->linenumber;
newfunc.stat |= FUNCTION_STATEMENT;
newfunc.moduleptr=GetCurrentModuleInformationFromBufferAddress(next->funcstart);		/* get module information for this function */
newfunc.type_int=next->type_int;

strncpy(newfunc.returntype,next->returntype,MAX_SIZE);

PushFunctionCallInformation(&newfunc);			/* push function information onto call stack */

currentfunction=FunctionCallStackTop;			/* point to function */
currentfunction->initialparameters=NULL;
currentfunction->saveinformation_top=NULL;
currentfunction->vars=NULL;

DeclareBuiltInVariables(NULL,NULL);			/* declare built-in variables */

parameters=next->parameters;

/* add variables from parameters */

for(count=0;count < returnvalue;count += 2) {
	if(parameters->type_int == VAR_NUMBER) {
		paramval.d=atof(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_STRING) {
		strncpy(&paramval.s,evaltokens[count],MAX_SIZE);
	}
	else if(parameters->type_int == VAR_INTEGER) {
		paramval.i=atoi(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_SINGLE) {
		paramval.f=atof(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_LONG) {
		paramval.l=atol(evaltokens[count]);
	}
	else if(parameters->type_int == VAR_BOOLEAN) {
		if(atoi(evaltokens[count]) > 1) {		/* Invalid value */
			SetLastError(TYPE_ERROR);

			PopFunctionCallInformation();
			return(-1);
		}

		paramval.b=atoi(evaltokens[count]);
	}

	if(parameters->type_int == VAR_UDT) {			/* user defined type */
		if(CreateVariable(parameters->varname,parameters->udt_type,split.x,split.y) == -1) {
			ReturnFromFunction();			/* set script's function return value */
			return(-1);
		}
	}
	else							/* built-in type */
	{
		if(CreateVariable(parameters->varname,BuiltInVariableTypes[parameters->type_int],1,1) == -1) {
			ReturnFromFunction();			/* set script's function return value */
			return(-1);
		}
	}

	/* Set parameter value */
	if(UpdateVariable(parameters->varname,NULL,&paramval,split.x,split.y,split.fieldx,split.fieldy) == -1) return(-1);

	/* set parameters->IsConstant after calling UpdateVariable() to make sure it will only be updated only once */

	var=GetVariablePointer(parameters->varname);		/* Get pointer to variable entry */
	if(var == NULL) return(-1);

	if(parameters->FunctionParameterIsConstant == TRUE) var->IsConstant=parameters->FunctionParameterIsConstant;

	NumberOfArguments++;

	/* add initial parameters displayed by stack trace */
	
	if(currentfunction->initialparameters == NULL) {
		currentfunction->initialparameters=calloc(1,sizeof(struct vartype));
		if(currentfunction->initialparameters == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		initialparameters=currentfunction->initialparameters;
	}
	else
	{
		initialparameters->next=calloc(1,sizeof(struct vartype));
		if(initialparameters->next == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		initialparameters=initialparameters->next;
	}

	var=GetVariablePointer(parameters->varname);
	if(var == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	memcpy(initialparameters,var,sizeof(struct vartype));

	parameters=parameters->next;
	if(parameters == NULL) break;

}

/* check if number of arguments matches number of parameters */
/*    Starts from 0 to allow for functions without parameters, but +1 to prevent off-by-one errors */

if( ((NumberOfArguments+1) < next->funcargcount) || ((returnvalue/2) > NumberOfArguments)) {
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
	    strncpy(tokens[count],buf,MAX_SIZE);
	}

	if(*tokens[count] == '0') {				/* octal number */
	   valptr=tokens[count];
	   valptr=valptr+2;

	   itoa(atoi_base(valptr,8),buf);
	   strncpy(tokens[count],buf,MAX_SIZE);
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

	  	if(CallFunction(tokens,count,countx) == -1) return(-1);	/* call the function */

		if(retval.has_returned_value == TRUE) {		/* function has returned value */
		  	get_return_value(&subst_returnvalue);

		  	if(subst_returnvalue.type == VAR_STRING) {		/* returning string */   
		  		sprintf(temp[outcount++],"%s",retval.val.s);
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
			else if(subst_returnvalue.type == VAR_BOOLEAN) {			/* returning boolean */
				sprintf(temp[outcount++],"%d",retval.val.b);
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
					strncpy(temp[outcount++],tokens[count],MAX_SIZE);
		      		}
				else
				{
	    	      			arraysize=(GetVariableXSize(split.name)*GetVariableYSize(split.name));

	    				if(((split.x*split.y) > arraysize) || arraysize < 0) {
						SetLastError(INVALID_ARRAY_SUBSCRIPT); /* Out of bounds */
						return(-1);
					}
	
			        	if(GetVariableValue(split.name,split.fieldname,split.x,split.y,&val,split.fieldx,split.fieldy) == -1) return(-1);
	
			  		strncpy(temp[outcount++],val.s,strlen(val.s));	/* copy value */
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
		else if(type == VAR_BOOLEAN) {   
		    sprintf(temp[outcount++],"%d",val.b);
	       	}
	 }
	else if (((char) *tokens[count] == '"') || IsSeperator(tokens[count],TokenCharacters) || IsNumber(tokens[count])) {
		strncpy(temp[outcount++],tokens[count],MAX_SIZE);
	}   
	else
	{
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	count += skiptokens;

	if(count >= end) break;
}

/* copy tokens */

memset(out,0,MAX_SIZE*MAX_SIZE);

for(count=0;count<outcount;count++) {
	strncpy(out[count],temp[count],MAX_SIZE);
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

for(count=start;count < end;count++) {
	size += (strlen(tokens[count])+1);
}

val->type=VAR_STRING;

val->s=calloc(1,size+2);			/* allocate output buffer for string and quote characters */
if(val->s == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

destptr=val->s;				/* point to output buffer */

*destptr++='"';		/* put " at start */

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"+") == 0) { 

		/* not a string literal or string variable */
		if(GetVariableType(tokens[count+1]) != VAR_STRING) {
		   	SetLastError(TYPE_ERROR);
			return(-1);
		}
	}
	else if((char) *tokens[count] == '"') {
		StripQuotesFromString(tokens[count],temp);	/* remove quotes from string */
		strncat(val->s,temp,strlen(temp));
	}
	else
	{
		SetLastError(SYNTAX_ERROR);
		return(-1);
	}
}

strncat(val->s,"\"",size);		/* put " at end */

SetLastError(0);
return(0);
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
	functioncallstack=calloc(1,sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */
	if(functioncallstack == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
	
	FunctionCallStackTop=functioncallstack;
}
else	
{
	NewTop=calloc(1,sizeof(FUNCTIONCALLSTACK));		/* allocate new entry */
	if(NewTop == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	NewTop->next=FunctionCallStackTop;			/* push onto top of stack */
	FunctionCallStackTop=NewTop;
}

strncpy(FunctionCallStackTop->name,func->name,MAX_SIZE);		/* copy information */
FunctionCallStackTop->callptr=func->callptr;
FunctionCallStackTop->startlinenumber=func->startlinenumber;
FunctionCallStackTop->currentlinenumber=func->startlinenumber;
FunctionCallStackTop->saveinformation_top=NULL;
FunctionCallStackTop->vars=func->vars;
FunctionCallStackTop->stat=func->stat;

if(GetFunctionPointer(func->name) != NULL) FunctionCallStackTop->initialparameters=GetFunctionPointer(func->name)->parameters;

FunctionCallStackTop->moduleptr=func->moduleptr;
strncpy(FunctionCallStackTop->returntype,func->returntype,MAX_SIZE);
FunctionCallStackTop->type_int=func->type_int;
FunctionCallStackTop->lastlooptype=func->lastlooptype;

currentfunction=FunctionCallStackTop;

//SetLastError(0);
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

FreeVariablesList(currentfunction->vars);	/* remove variables */

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

memcpy(var,currentfunction->vars,sizeof(struct vartype));

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

memcpy(var,findvar,sizeof(struct vartype));

findvar=findvar->next;
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
	udt=calloc(1,sizeof(UserDefinedType));		/* add link to the end */
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

	last->next=calloc(1,sizeof(UserDefinedType));
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

dest->field=calloc(1,sizeof(UserDefinedTypeField));
if(dest->field == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

sourcenext=source->field;
destnext=dest->field;

while(sourcenext != NULL) {
/* copy rather than duplicate */

	strncpy(destnext->fieldname,sourcenext->fieldname,MAX_SIZE);
	destnext->xsize=sourcenext->xsize;
	destnext->ysize=sourcenext->ysize;
	destnext->type=sourcenext->type;

	if(sourcenext->type == VAR_NUMBER) {		/* double precision */			
		destnext->fieldval=calloc(1,(sourcenext->xsize*sizeof(double))*(sourcenext->ysize*sizeof(double)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(double))*(sourcenext->ysize*sizeof(double)));
	}
	else if(sourcenext->type == VAR_SINGLE) {		/* single precision */			
		destnext->fieldval=calloc(1,(sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(float))*(sourcenext->ysize*sizeof(float)));
	}
	if(sourcenext->type == VAR_INTEGER) {		/* integer */
	destnext->fieldval=calloc(1,(sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(int))*(sourcenext->ysize*sizeof(int)));
	}
	if(sourcenext->type == VAR_LONG) {		/* long */
	destnext->fieldval=calloc(1,(sourcenext->xsize*sizeof(long long))*(sourcenext->ysize*sizeof(long long)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		memcpy(destnext->fieldval,sourcenext->fieldval,(sourcenext->xsize*sizeof(long long))*(sourcenext->ysize*sizeof(long long)));
	}
	else if(sourcenext->type == VAR_STRING) {	/* string */			
		destnext->fieldval=calloc(1,((sourcenext->xsize*MAX_SIZE)*(sourcenext->ysize*MAX_SIZE)*sizeof(char *)));
		if(destnext->fieldval == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

	/* copy strings in array */

		for(count=0;count<(sourcenext->xsize*sourcenext->ysize);count++) {
	 		if(sourcenext->fieldval->s[count] == NULL) {
				sourcenext->fieldval[count].s=calloc(1,strlen(destnext->fieldval[count].s));
				if(sourcenext->fieldval[count].s == NULL) {
					SetLastError(NO_MEM);
					return(-1);
				}

				strncpy(destnext->fieldval[count].s,sourcenext->fieldval->s[count],MAX_SIZE);
			}
		}
	 }

	 destnext->next=calloc(1,sizeof(UserDefinedTypeField));
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
			val->s=calloc(1,strlen(udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s)+1);	/* allocate string */
			if(val->s == NULL) {
				SetLastError(NO_MEM);
				return(-1);
			}

		        strncpy(val->s,udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s,strlen(udtfield->fieldval[(fieldy*next->ysize)+(next->xsize*fieldx)].s));

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
 		else if(udtfield->type == VAR_BOOLEAN) {
		        val->b=udtfield->fieldval[(next->ysize*fieldy)+(next->xsize*fieldx)].b;
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
*  Check if built-in variable type
* 
*  In: type	variable type name
* 
*  Returns variable type, -1 otherwise
* 
*/

int IsBuiltInVariableType(char *type) {
int count=0;

while(BuiltInVariableTypes[count] != NULL) {

	if(strcmpi(BuiltInVariableTypes[count],type) == 0) return(count);
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

char *GetCurrentFunctionName(void) {
return(currentfunction->name);
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

currentfunction->stat &= ~flags;
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
if(currentfunction == NULL) return(0);

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
	currentfunction->saveinformation_top=calloc(1,sizeof(SAVEINFORMATION));		/* allocate new entry */

	if(currentfunction->saveinformation_top == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}
}
else
{
	newinfo=calloc(1,sizeof(SAVEINFORMATION));		/* allocate new entry */
	if(newinfo == NULL) {
		SetLastError(NO_MEM);
		return(-1);
	}

	newinfo->next=currentfunction->saveinformation_top;	/* put entry at top of stack */
	currentfunction->saveinformation_top=newinfo;
}

currentfunction->saveinformation_top->bufptr=GetCurrentBufferPosition();
currentfunction->saveinformation_top->linenumber=GetCurrentFunctionLine();
currentfunction->saveinformation_top->next=NULL;

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

if(currentfunction->saveinformation_top != NULL) {
	free(oldtop);		/* free entry */
}
else
{
	return(-1);
}

return(0);
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
vars_t *savenext;	

while(next != NULL) {
	/* If it's a string variable, free it */
	if(next->type_int == VAR_STRING) {
		for(count=0;count < (next->xsize*next->ysize);count++) {
			if(next->val[count].s != NULL) free(next->val[count].s);
		}
	}

	if(next->val != NULL) free(next->val);	/* free values */

	savenext=next->next;

	free(next);

	next=savenext;
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
SAVEINFORMATION *old;

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
	FreeVariablesList(callstacknext->initialparameters);

	savenext=callstacknext->saveinformation_top			;

	while(savenext != NULL) {
		old=savenext->next;
		free(savenext);

		savenext=old;
	}
	
	callstackptr=callstacknext->next;

	free(callstacknext);
	callstacknext=callstackptr;
}

functioncallstack=NULL;
return;
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

/*
 * Check if seperator
 *
 * In: token		Token to check
       sep		Seperator characters to check against
 *
 * Returns TRUE or FALSE
 *
 */
int IsSeperator(char *token,char *sep) {
char *SepPtr;

if(*token == 0) return(TRUE);
	
SepPtr=sep;

while(*SepPtr != 0) {
	if((char) *SepPtr++ == (char) *token) return(TRUE);
}

if(IsStatement(token) == TRUE) return(TRUE);

return(FALSE);
}

