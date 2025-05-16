/*  XScript Version 0.0.1
   (C) Matthew Boote 2020

   This file is part of XScript.7

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
#include <string.h>
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"
#include "debugmacro.h"

int last_error=0;

char *errs[] = { "No error",\
		 "File not found",\
		 "Invalid expression",\
		 "IF statement without ENDIF",\
		 "FOR statement without NEXT",\
		 "WHILE without WEND",\
		 "ELSE or ELSEIF without IF",\
		 "ENDIF without IF",\
		 "ENDFUNCTION without FUNCTION",\
		 "Invalid variable name",\
		 "Out of memory",\
		 "EXIT outside of FOR or WHILE loop",\
		 "Read error",
		 "Syntax error",\
		 "Error calling library function",\
		 "Invalid statement",\
		 "Cannot declare a function inside another function",\
		 "FUNCTION without ENDFUNCTION",\
		 "NEXT without FOR",
		 "WEND without WHILE",\
		 "Duplicate function name",\
		 "Invalid number of arguments to function call",\
		 "Invalid array subscript",\
		 "Type mismatch",\
		 "Invalid variable type",\
		 "EXIT FOR without FOR",\
		 "Missing XSCRIPT_MODULE_PATH path",\
		 "Variable already exists",\
		 "Variable or function not defined",\
		 "EXIT WHILE without WHILE",\
		 "FOR without NEXT",\
		 "User-defined type already exists",\
		 "TYPE without ENDTYPE",\
		 "Field in user-defined type does not exist",\
		 "Breakpoint not found",\
		 "No program loaded",\
		 "Invalid value",\
		 "Include file without main file",\
		 "TRY without CATCH",\
		 "TRY without ENDTRY",\
		 "CATCH without TRY",\
		 "ENDTRY without TRY",\
		 "No program currently running",\
		 "Not an array",\
		 "Missing array subscript",\
		 "User-defined type does not exist",\
		 "TYPE without ENDTYPE",\
		 "Command can only be used in interactive mode",\
		 "IMPORT statement must precede statements",\

};

/*
 * Display error message
 *
 * In: errornumber		Error number
 *
 * Returns: Nothing
   *
 */

void PrintError(int errornumber) {
char *filename[MAX_SIZE];
MODULES module;
FUNCTIONCALLSTACK *stack;

if(errornumber == 0) return;

GetCurrentModuleInformationFromBufferAddress(&module);		/* get information about current module */

if(GetIsFileLoadedFlag() == TRUE) {
	printf("Error %d: %s\n",errornumber,errs[errornumber]);
	PrintBackTrace();
}
else
{

	printf("Error %d: %s\n",errornumber,errs[errornumber]); 
}

asm("int $3");

if(GetInteractiveModeFlag() == FALSE) {
	exit(errornumber); 		/* If running from non-interactive source, exit */
}

return;
}


/*
 * Set last error
 *
 * In: errornumber	Error number
 *
 * Returns: nothing
 *
 */
void SetLastError(int errornumber) {
varval errval;
char *errfunc[MAX_SIZE];

last_error=errornumber;

errval.i=errornumber;				/* update error number */
UpdateVariable("ERR","",&errval,0,0);

errval.i=GetCurrentFunctionLine();		/* update error line */
UpdateVariable("ERRL","",&errval,0,0);

GetCurrentFunctionName(errfunc);		/* get faulting function */

//errval.s=malloc(strlen(errfunc));
//if(errval.s == NULL) return;

//strcpy(errval.s,errfunc);

//UpdateVariable("ERRFUNC","",&errval,0,0);

//free(errval.s);
}

int GetLastError(void) {
return(last_error);
}

