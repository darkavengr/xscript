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
#include <setjmp.h>
#include "errors.h"
#include "size.h"
#include "variablesandfunctions.h"
#include "debug.h"

BREAKPOINT *breakpoints;
BREAKPOINT *breakpointend;

/*
 * Set breakpoint
 *
 * In: linenumber	Line number to set breakpoint at
 *     functionname	Function to set breakpoint in
 *
 * Returns error number on error or 0 on success
 *
 */
int set_breakpoint(int linenumber,char *functionname) {
 BREAKPOINT *next;

/* check if breakpoint set */

 if(breakpoints == NULL) {
   breakpoints=malloc(sizeof(BREAKPOINT));
   breakpointend=breakpoints;
 }
 else
 {
   next=breakpoints;
 
   while(next != NULL) {
    if((next->linenumber == linenumber) && (strcmp(next->functionname,functionname) == 0)) {	/* breakpoint already set */  
	SetLastError(BREAKPOINT_DOES_NOT_EXIST);
	return(-1);
    }

    next=next->next;
   }

   breakpointend->next=malloc(sizeof(BREAKPOINT));
   breakpointend=breakpointend->next;
 }

 breakpointend->linenumber=linenumber;
 strcpy(breakpointend->functionname,functionname);

 return;
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


    if((next->linenumber == linenumber) && (strcmp(next->functionname,functionname) == 0)) {	/* breakpoint already set */  

	    if(next == breakpoints) {		/* head */
		free(breakpoints);

		breakpoints=NULL;
		return;
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

 return;
}

/*
 * Check breakpoint
 *
 * In: linenumber	Line number to set breakpoint at
 *     functionname	Function to set breakpoint in
 *
 * Returns error number on error or 0 on success
 *
 */
int check_breakpoint(int linenumber,char *functionname) {
 BREAKPOINT *next;
 next=breakpoints;
 
 while(next != NULL) {

    if((next->linenumber == linenumber) && (strcmp(next->functionname,functionname) == 0)) return(TRUE);
	    
    next=next->next;
 }

 return(FALSE);
}

/*
 * Print variable
 *
 * In: var		Variable to print
 *
 * Returns error number on error or 0 on success
 *
 */
void PrintVariable(vars_t *var) {
int count;
int vartype=0;
int ycount=0;
int xcount=0;
int yinc=0;
int xinc=0;

printf("%s",var->varname);

 for(count=0;count<12-strlen(var->varname);count++) {
   printf(" ");
 }

 printf(" = ");

 vartype=GetVariableType(var->varname);

 yinc=var->ysize;
 if(yinc == 0) yinc++;

 xinc=var->xsize;
 if(xinc == 0) xinc++;

 printf("size=%d %d\n",var->xsize,var->ysize);

 if((var->xsize >= 1) || (var->ysize >= 1)) printf("(");

 for(count=0;count<(xinc*yinc);count++) {
	 switch(vartype) {
	  case VAR_NUMBER:				/* double precision */			
		printf("%.6g",var->val[count].d);
	        break;

	  case VAR_STRING:				/* string */
		printf("\"%s\"",var->val[count].s);
	        break;

	  case VAR_INTEGER:	 			/* integer */
		printf("%d",var->val[count].i);
        	break;

	  case VAR_SINGLE:				/* single */	     
		printf("%f",var->val[count].f);
	        break;
  	  }

	  if(count < ((xinc*yinc)-1)) printf(",");		 
	
  }

  if((var->xsize >= 1) || (var->ysize >= 1)) printf(")");
  
  printf("\n");
}

/*
 * List variables
 *
 * In: name		Variable to list. Empty string to print all variables
 *
 * Returns: error number on error or 0 on success
 *
 */
void list_variables(char *name) {
vars_t var;

if(strlen(name) > 0) {
	if(FindVariable(name,&var) == -1) {
		SetLastError(VARIABLE_DOES_NOT_EXIST);	
		return(-1);
	}

	PrintVariable(&var);
	return;
}

FindFirstVariable(&var);
	
do {
 PrintVariable(&var);
 } while(FindNextVariable(&var) != -1);

}

/*
 * Set command-line arguments
 *
 * In: argp		Point to arguments
 *
 * Returns: Nothing
 *
 */
void SetArguments(char *argp[MAX_SIZE][MAX_SIZE],int argcount) {
varval cmdargs;
int count;

CreateVariable("argcount","INTEGER",0,0);

cmdargs.i=argcount;
UpdateVariable("argcount",NULL,&cmdargs,0,0);

CreateVariable("args","STRING",argcount,1);

cmdargs.s=malloc(MAX_SIZE);

for(count=0;count<argcount;count++) {
	strcpy(cmdargs.s,&argp[count]);

	UpdateVariable("args",NULL,&cmdargs,count,0);
}

return;
}

