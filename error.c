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
#include <string.h>
#include "size.h"
#include "variablesandfunctions.h"
#include "debugmacro.h"

char *errs[] = { "No error",\
		 "File not found",\
		 "Missing parameters in statement",\
		 "Invalid expression",\
		 "IF statement without ENDIF",\
		 "FOR statement without NEXT",\
		 "WHILE without WEND",\
		 "ELSE without IF",\
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
		 "ENDFUNCTION without FUNCTION",\
		 "NEXT without FOR",
		 "WEND without WHILE",\
		 "Duplicate function name",\
		 "Too few arguments to function call",\
		 "Invalid array subscript",\
		 "Type mismatch",\
		 "Invalid variable type",\
		 "EXIT FOR without FOR",\
		 "ELSEIF without IF",\
		 "Missing XSCRIPT_MODULE_PATH path",\
		 "Variable already exists",\
		 "Variable not found",\
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
};

/*
 * Display error
 *
 * In: err			Error number
 *
 * Returns: Nothing
 *
 */

void PrintError(int errornumber) {
char *functionname[MAX_SIZE];

GetCurrentFunctionName(functionname);

if(GetIsFileLoadedFlag() == TRUE) {
	printf("Error %d in function %s (line %d): %s\n",errornumber,functionname,GetCurrentFunctionLine(),errs[errornumber]);
}
else
{

	printf("Error %d: %s\n",errornumber,errs[errornumber]); 
}

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

errval.i=errornumber;				/* update error number */
UpdateVariable("ERR","",&errval,0,0);

errval.i=GetCurrentFunctionLine();		/* update error line */
UpdateVariable("ERRL","",&errval,0,0);

GetCurrentFunctionName(errfunc);		/* get faulting function */

errval.s=malloc(strlen(errfunc));
if(errval.s == NULL) return;

strcpy(errval.s,errfunc);

UpdateVariable("ERRFUNC","",&errval,0,0);

free(errval.s);
}

