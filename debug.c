#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <setjmp.h>
#include "define.h"

int set_breakpoint(int linenumber,char *functionname);
int clear_breakpoint(int linenumber,char *functionname);
int check_breakpoint(int linenumber,char *functionname);
void PrintVariable(vars_t *var);
void list_variables(char *name);

BREAKPOINT *breakpoints;
BREAKPOINT *breakpointend;

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
	printf("Breakpoint already set\n"); 
	return;
    }

    next=next->next;
   }

   breakpointend->next=malloc(sizeof(BREAKPOINT));
   breakpointend=breakpointend->next;
 }

 breakpointend->linenumber=linenumber;
 strcpy(breakpointend->functionname,functionname);
}


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

}

int check_breakpoint(int linenumber,char *functionname) {
 BREAKPOINT *next;
 next=breakpoints;
 
 while(next != NULL) {

    if((next->linenumber == linenumber) && (strcmp(next->functionname,functionname) == 0)) return(TRUE);
	    
    next=next->next;
 }

 return(FALSE);
}

void PrintVariable(vars_t *var) {
int count;
int vartype=0;
int ycount=0;
int xcount=0;
int yinc=0;
int xinc=0;

printf("%s",var->varname);

// asm("int $3");

 for(count=0;count<12-strlen(var->varname);count++) {
   printf(" ");
 }

 printf(" = ");

 vartype=GetVariableType(var->varname);

 yinc=var->ysize;
 if(yinc == 0) yinc++;

 xinc=var->xsize;
 if(xinc == 0) xinc++;

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

void list_variables(char *name) {
vars_t var;

if(strlen(name) > 0) {
	if(FindVariable(name,&var) == -1) {
		PrintError(VARIABLE_DOES_NOT_EXIST);	
		return;
	}

	PrintVariable(&var);
	return;
}

FindFirstVariable(&var);
	
do {
 PrintVariable(&var);
 } while(FindNextVariable(&var) != -1);

}

