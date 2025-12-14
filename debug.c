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

/* debug functions */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "errors.h"
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"
#include "debug.h"
#include "dofile.h"

extern char *vartypenames[];

BREAKPOINT *breakpoints;
BREAKPOINT *breakpointend;
BREAKPOINT *findptr;

/*
 * Set breakpoint
 *
 * In: linenumber	Line number to set breakpoint at
 *     functionname	Optional function to set breakpoint in
 *
 * Returns error number on error or 0 on success
 *
 */
int set_breakpoint(int linenumber,char *functionname) {
BREAKPOINT *next;
char *funcname[MAX_SIZE];

if(strlen(functionname) == 0) {		/* no function */
	strncpy(funcname,GetCurrentFunctionName(),MAX_SIZE);
}
else
{
	strncpy(funcname,functionname,MAX_SIZE);
}

if(breakpoints == NULL) {
	breakpoints=calloc(1,sizeof(BREAKPOINT));
	breakpointend=breakpoints;
}
else
{
	next=breakpoints;
 
	while(next != NULL) {
		/* breakpoint already set */
		if((next->linenumber == linenumber) && (strncmp(next->functionname,funcname,MAX_SIZE) == 0)) break;

    		next=next->next;
   	}

	if(next == NULL) {	/* add breakpoint if it does not exist */
		breakpointend->next=calloc(1,sizeof(BREAKPOINT));
		breakpointend=breakpointend->next;
	}
}

breakpointend->linenumber=linenumber;
strncpy(breakpointend->functionname,funcname,MAX_SIZE);

return(0);
}

/*
 * Clear breakpoint
 *
 * In: linenumber	Line number to set breakpoint at
 *     functionname	Function to set breakpoint in
 *
 * Returns error number on error or 0 on success
 *
 */
int clear_breakpoint(int linenumber,char *functionname) {
 BREAKPOINT *next;
 BREAKPOINT *last;

 next=breakpoints;
 
 while(next != NULL) {
    last=next;


    if((next->linenumber == linenumber) && (strncmp(next->functionname,functionname,MAX_SIZE) == 0)) {	/* breakpoint found */  

	    if(next == breakpoints) {		/* head */
		free(breakpoints);

		breakpoints=NULL;
		return(0);
	    }
	    else if(next->next == NULL) {	/* end */
		free(next);
	    }
	    else
	    {
		last->next=next->next;		/* middle */
		free(next);
	    }
    }
	    
    next=next->next;
 }
 
SetLastError(BREAKPOINT_DOES_NOT_EXIST); 
return(-1);
}

/*
 * Check breakpoint
 *
 * In: linenumber	Line number to set breakpoint at
 *     functionname	Function to set breakpoint in
 *
 * Returns TRUE or FALSE
 *
 */
int check_breakpoint(int linenumber,char *functionname) {
BREAKPOINT *next;
next=breakpoints;
 
while(next != NULL) {
	printf("check_breakpoint() %s %s %d %d\n",next->functionname,functionname,next->linenumber,linenumber);

	if((next->linenumber == linenumber) && (strncmp(next->functionname,functionname,MAX_SIZE) == 0)) return(TRUE);
	    
	next=next->next;
}

return(FALSE);
}

/*
 * Print variable
 *
 * In: var		Variable to print
 *
 * Returns: nothing
 *
 */
void PrintVariableValue(vars_t *var) {
int vartype=0;
int ycount=0;
int xcount=0;
int xsize=GetVariableXSize(var->varname);
int ysize=GetVariableYSize(var->varname);
char *varprefixname[MAX_SIZE];
int padlength;

padlength=strlen(var->varname)+1;	/* get variable name and = to get length */

vartype=GetVariableType(var->varname);

for(ycount=0;ycount != ysize;ycount++) {

	/* print ( before arrays and pad out with spaces on subsequent lines */

	if((GetVariableXSize(var->varname) > 1) || (GetVariableYSize(var->varname) > 1)) {
		if(ycount == 0) {
			printf("(");
		}
		else
		{ 
			printf("%*s",padlength+1,"(");
		}
	}
		
	for(xcount=0;xcount != xsize;xcount++) {
		
		switch(vartype) {

			 case VAR_NUMBER:				/* double precision */
				printf("%.6g",var->val[(xcount*xsize)+ycount].d);
			        break;

			  case VAR_STRING:				/* string */
				if(var->val[((xcount*xsize)+ycount)].s == NULL) {	/* empty string */
					printf("\"\"");
				}
				else
				{
					printf("\"%s\"",var->val[(xcount*xsize)+ycount].s);
				}

			        break;

			  case VAR_INTEGER:	 			/* integer */
				printf("%d",var->val[(xcount*xsize)+ycount].i);
        			break;

			  case VAR_SINGLE:				/* single */	     
				printf("%f",var->val[(xcount*xsize)+ycount].f);
			        break;
  		}
	
		  if(xcount < (xsize-1)) printf(",");	
	  }

	if((GetVariableXSize(var->varname) > 1) || (GetVariableYSize(var->varname) > 1)) printf(")\n");

}

if((GetVariableXSize(var->varname) <= 1) || (GetVariableYSize(var->varname) <= 1)) printf("\n");
}

/*
 * List variables
 *
 * In: name		Variable to list. Empty string to print all variables
 *
 * Returns: -1 on error or 0 on success
 *
 */
int list_variables(char *name) {
vars_t var;
int padcount;

/* list specific variable */

if(strlen(name) > 0) {

	if(GetVariablePointer(name) == NULL) {
		SetLastError(VARIABLE_OR_FUNCTION_DOES_NOT_EXIST);	
		return(-1);
	}

	printf("%s=",var.varname);
	PrintVariableValue(&var);

	SetLastError(0);	
	return(0);
}

/* list all variables */

FindFirstVariable(&var);
	
do {
	printf("%s=",var.varname);

	PrintVariableValue(&var);

} while(FindNextVariable(&var) != -1);

SetLastError(0);	
return(0);
}

/*
 * Find first breakpoint
 *
 * In: Nothing
 *
 * Returns: Pointer to first breakpoint
 *
 */
BREAKPOINT *GetBreakpointsPointer(void) {
findptr=breakpoints;
return(findptr);
}


