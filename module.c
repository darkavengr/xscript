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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "errors.h"
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"
#include "dofile.h"

extern functionreturnvalue retval;
extern char *vartypenames[];

/*
 * Module functions
 *
 */

MODULES *modules=NULL;
MODULES *modules_end=NULL;

/*
 * Load module
 *
 * In: modulename	Module filename
 *
 * Returns error number on error or result on success
 *
 * Returns -1 on error or 0 on success
 */

int AddModule(char *filename) {
MODULES ModuleEntry;

if(GetModuleHandle(filename) != NULL) {		/* module already loaded */
	SetLastError(NO_ERROR);
	return(0);
}

ModuleEntry.dlhandle=LoadModule(filename);	/* load module */
if(ModuleEntry.dlhandle == NULL) return(-1);

/* add module entry */
strncpy(ModuleEntry.modulename,filename,MAX_SIZE);
ModuleEntry.flags=MODULE_BINARY;
AddToModulesList(&ModuleEntry);		/* add to modules list */

SetLastError(NO_ERROR);
return(0);
}

/*
 * Get module handle
 *
 * In: module	Module name
 *	
 * Returns module handle or NULL on error
 *
 */

void *GetModuleHandle(char *module) {
MODULES *next=modules;

/* search through linked list */

while(next != NULL) {
	if(strcmpi(next->modulename,module) == 0) return(next->dlhandle);		/* found module */
 
	next=next->next;
}

return(NULL);
}

/*
 * Get point to module list entry
 *
 * In: module	Module name
 *	
 * Returns: pointer to module entry or NULL.
 *
 */

MODULES *GetModuleEntry(char *module) {
MODULES *next=modules;

/* search through linked list */

while(next != NULL) {
	if(strcmpi(next->modulename,module) == 0) return(next);		/* found module */
 
	next=next->next;
}

return(NULL);
}

/*
 * Free modules list
 *
 * In: Nothing
 *	
 * Returns: Nothing
 *
 */

void FreeModulesList(void) {
MODULES *next=modules;
MODULES *nextptr;

while(next != NULL) {
	nextptr=next->next;

	if(next->flags & MODULE_SCRIPT) {
		if(next->StartInBuffer != NULL) free(next->StartInBuffer);		/* free script module buffer */
	}

	if(next->dlhandle != NULL) dlclose(next->dlhandle);		/* release binary module handle */

	free(next);

	next=nextptr;
}

modules=NULL;
return;
}

int AddToModulesList(MODULES *entry) {
if(modules == NULL) {			/* first in list */
	modules=calloc(1,sizeof(MODULES));
	if(modules == NULL) {
		SetLastError(NO_MEM);
	  	return(-1);
	}

	modules_end=modules;
	modules_end->last=NULL;
}
else
{
	modules_end->next=calloc(1,sizeof(MODULES));                         	
	if(modules_end->next == NULL) {
 		SetLastError(NO_MEM);
 		return(-1);
	}

	modules_end=modules_end->next;
}

memcpy(modules_end,entry,sizeof(MODULES));
modules_end->next=NULL;

SetLastError(0);
return(0);
}

/*
 * Get module entry from start address
 *
 * In: Nothing
 *	
 * Returns: Pointer to module information or NULL on error
 *
 */

MODULES *GetCurrentModuleInformationFromBufferAddress(char *address) {
MODULES *next=modules;

/* search through module list for buffer address */

while(next != NULL) {
//	printf("%s %lX %lX\n",next->modulename,next->StartInBuffer,next->EndInBuffer);

	if((address >= next->StartInBuffer) && (address <= next->EndInBuffer)) return(next);	/* found entry */
 
	next=next->next;
}

return(NULL);
}

/*
 * Call binary module
 *
 * In:  functionname	Function name
	modulename	Module name
 *	paramcount	Number of parameters
 *	parameters	Function parameters
 *	result		Result variable
 *
 * Returns: 0 on success or -1 on error
 *
 */
int CallModule(char *functionname,char *modulename,int paramcount,char *parameters[MAX_SIZE][MAX_SIZE],libraryreturnvalue *result) {
int count;	
void (*callptr)(int,vars_t *,libraryreturnvalue *result);
FUNCTIONCALLSTACK modulefunctioncall;
UserDefinedType *udtptr;
vars_t *varptr;
vars_t *paramvars=NULL;
int paramstc;
char *parameters_subst[MAX_SIZE][MAX_SIZE];
char *stodarg;
bool HasReturnedError=FALSE;
int unwindcount;
char *strbuf[MAX_SIZE];
void *modhandle;

paramstc=SubstituteVariables(0,paramcount,parameters,parameters_subst);	/* substitute values */

paramvars=calloc(paramcount,sizeof(vars_t));	/* allocate parameters */
if(paramvars == NULL) {
	SetLastError(NO_MEM);
	return(-1);
}

/* get variable values */

for(count=0;count < paramstc;count++) {
	paramvars[count].type_int=GetVariableType(parameters[count]);		/* get variable type */

	paramvars[count].val=calloc(1,sizeof(vars_t));		/* allocate value */
	if(paramvars[count].val == NULL) {
		HasReturnedError=TRUE;
		break;
	}

	if(paramvars[count].type_int == VAR_SINGLE) {
		paramvars[count].val->f=atof(parameters_subst[count]);
	}
	else if(paramvars[count].type_int == VAR_NUMBER) {
		paramvars[count].val->d=strtod(parameters_subst[count],&stodarg);
	}
	else if(paramvars[count].type_int == VAR_INTEGER) {
		paramvars[count].val->i=atoi(parameters_subst[count]);
	}
	else if(paramvars[count].type_int == VAR_LONG) {
		paramvars[count].val->l=atol(parameters_subst[count]);
	}
	else if(paramvars[count].type_int == VAR_BOOLEAN) {
		paramvars[count].val->b=atoi(parameters_subst[count]);
	}
	else if(paramvars[count].type_int == VAR_STRING) {
		paramvars[count].val->s=calloc(1,strlen(parameters_subst[count]));				/* allocate parameter */
		if(paramvars[count].val->s == NULL) {
			HasReturnedError=TRUE;
			break;
		}

		StripQuotesFromString(parameters[count],paramvars[count].val->s);	/* remove quotes from string parameter */
	}
	else if(paramvars[count].type_int == VAR_ANY) {
		paramvars[count].val->a=(void *) atoi(parameters_subst[count]);
	}
	else if(paramvars[count].type_int == VAR_UDT) {
		varptr=GetVariablePointer(parameters[count]);		/* Get variable pointer */	
		if(varptr == NULL) {
			HasReturnedError=TRUE;
			break;
		}

		paramvars[count].udt=udtptr=varptr->udt;		/* point to UDT */
	}
}

/* call binary function and return a value */

if(HasReturnedError == FALSE) {
	modhandle=LoadModule(modulename);		/* open handle */
	if(modhandle == NULL) {
		SetLastError(FILE_NOT_FOUND);
		return(-1);
	}
		
	callptr=GetLibraryFunctionAddress(modhandle,functionname);		/* get pointer to library function */
	if(callptr == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);

		HasReturnedError=TRUE;
	}
	else
	{
		callptr(paramcount,paramvars,result);		/* call library function */

		/* place string in quotes */
	
		if(GetVariableType(result->name) == VAR_STRING) {
			snprintf(strbuf,MAX_SIZE,"\"%s\"",result->val.s);
			strncpy(result->val.s,strbuf,MAX_SIZE);
		}

		/* update result variable */
		if(UpdateVariable(result->name,result->fieldname,&result->val,result->x,result->y,result->fieldx,result->fieldy) == -1) {
			HasReturnedError=TRUE;
		}

	}
}

/* free values */

for(unwindcount=0;unwindcount < count;unwindcount++) {
	if(paramvars[unwindcount].val != NULL) {		/* has value */
		if((paramvars[unwindcount].val != NULL) && (paramvars[unwindcount].type_int == VAR_STRING)) { /* is string */
			free(paramvars[unwindcount].val->s);	/* free */
		}
	
		free(paramvars[unwindcount].val);	/* free value */
	}
}

free(paramvars);			/* free parameters */

if(HasReturnedError == FALSE) {
	SetLastError(NO_ERROR);
	return(0);
}

return(-1);
}

void InitalizeModules(void) {
modules=NULL;
}

