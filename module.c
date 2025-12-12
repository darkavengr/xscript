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
	modules=malloc(sizeof(MODULES));
	if(modules == NULL) {
		SetLastError(NO_MEM);
	  	return(-1);
	}

	modules_end=modules;
	modules_end->last=NULL;
}
else
{
	modules_end->next=malloc(sizeof(MODULES));                         	
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
 *	result		Result variable
 *	parameters	Function parameters
 *
 * Returns: 0 on success or -1 on error
 *
 */
int CallModule(char *functionname,char *modulename,int paramcount,varsplit *result,char *parameters[MAX_SIZE][MAX_SIZE]) {
int count;
int vartype;
void *varptrs[MAX_SIZE];
int (*callptr_integer)(void *,void *,void *,void *,void *,void *,void *,void *);
double (*callptr_double)(void *,void *,void *,void *,void *,void *,void *,void *);
float (*callptr_single)(void *,void *,void *,void *,void *,void *,void *,void *);
long (*callptr_long)(void *,void *,void *,void *,void *,void *,void *,void *);
char *(*callptr_string)(void *,void *,void *,void *,void *,void *,void *,void *);
UserDefinedType *(*callptr_udt)(void *,void *,void *,void *,void *,void *,void *,void *);
FUNCTIONCALLSTACK modulefunctioncall;
varval resultvar;
UserDefinedType *resultudt;
int result_type=GetVariableType(result->name);
double tempdouble[MAX_SIZE];
int tempint[MAX_SIZE];
float tempsingle[MAX_SIZE];
long int templong[MAX_SIZE];
char *tempstring[MAX_SIZE];
char *stodarg;
char *parameters_subst[MAX_SIZE][MAX_SIZE];
int doublecount;
int singlecount;
int integercount;
int stringcount;
int longcount;
vars_t *paramvar;

AddModule(modulename);		/* add module */

SubstituteVariables(0,paramcount,parameters,parameters_subst);	/* substitute values */

/* get variable values */

for(count=0;count<paramcount;count++) {
	vartype=GetVariableType(parameters[count]);		/* get variable type */

	if(vartype == VAR_SINGLE) {
		varptrs[count]=malloc(sizeof(float));				/* allocate parameter */
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		tempsingle[singlecount]=atof(parameters_subst[count]);
		varptrs[count]=&tempsingle[singlecount];
	}
	else if(vartype == VAR_NUMBER) {
		varptrs[count]=malloc(sizeof(double));		/* allocate parameter */
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		tempdouble[doublecount]=strtod(parameters_subst[count],&stodarg);
		varptrs[count]=&tempdouble[doublecount];
	}
	else if(vartype == VAR_INTEGER) {
		varptrs[count]=malloc(sizeof(int));				/* allocate parameter */
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		tempint[integercount]=atoi(parameters_subst[count]);
		varptrs[count]=&tempint[integercount];
	}
	else if(vartype == VAR_LONG) {
		varptrs[count]=malloc(sizeof(long));				/* allocate parameter */
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		templong[longcount]=atol(parameters_subst[count]);
		varptrs[count]=&templong[longcount];
	}
	else if(vartype == VAR_STRING) {
		varptrs[count]=malloc(strlen(parameters[count]));
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		StripQuotesFromString(parameters[count],varptrs[count]);	/* remove quotes from string parameter */	
	}
	else if(vartype == VAR_UDT) {
		varptrs[count]=malloc(sizeof(UserDefinedType));				/* allocate parameter */
		if(varptrs[count] == NULL) {
			SetLastError(NO_MEM);
			return(-1);
		}

		paramvar=GetVariablePointer(parameters[count]);		/* Get variable pointer */
		if(paramvar == NULL) {
			SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
			return(-1);
		}

		memcpy(varptrs[count],paramvar->udt,sizeof(UserDefinedType));
	}
}

/* call binary function and return a value */

resultvar.type=result_type;		/* set result type */

if(result_type == VAR_SINGLE) {
	callptr_single=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_single == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	resultvar.f=callptr_single(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]);
}
else if(result_type == VAR_NUMBER) {
	callptr_double=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_double == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	resultvar.d=callptr_double(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]);
}
else if(result_type == VAR_INTEGER) {	
	callptr_integer=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_integer == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	resultvar.i=callptr_integer(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]);

	printf("Called function and returned integer\n");
}
else if(result_type == VAR_LONG) {
	callptr_long=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_long == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	resultvar.l=callptr_long(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]);
}
else if(result_type == VAR_STRING) {
	callptr_string=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_string == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	resultvar.s=malloc(MAX_SIZE);
	if(resultvar.s == NULL) {
		SetLastError(NO_MEM);
	}

	strcpy(resultvar.s,callptr_string(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]));
}
else if(result_type == VAR_UDT) {
	callptr_udt=GetLibraryFunctionAddress(GetModuleHandle(modulename),functionname);
	if(callptr_udt == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	paramvar=GetVariablePointer(result->name);		/* Get variable pointer */
	if(paramvar == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);
		return(-1);
	}

	
	if(paramvar->udt == NULL) {	/* allocate UDT */
		paramvar->udt=malloc(sizeof(UserDefinedType));

		if(paramvar->udt == NULL)
			SetLastError(NO_MEM);
			return(-1);
		}

	memcpy(paramvar->udt,callptr_udt(varptrs[0],varptrs[1],varptrs[2],varptrs[3],varptrs[4],varptrs[5],varptrs[6],varptrs[7]),sizeof(UserDefinedType));

	SetLastError(NO_ERROR);
	return(0);
}

/* update result variable */
if(UpdateVariable(result->name,result->fieldname,&resultvar,result->x,result->y,result->fieldx,result->fieldy) == -1) return(-1);

SetLastError(NO_ERROR);
return(0);
}

void InitalizeModules(void) {
modules=NULL;
}

