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

/* statements definitions */

#include <stdio.h>
#include "size.h"
#include "variablesandfunctions.h"
#include "dofile.h"
#include "statements.h"
#include "errors.h"

/* statements */
			  
statement statements[] = {
	     { "IF","ENDIF",&if_statement,TRUE},\
	     { "ELSE",NULL,&else_statement,FALSE},\
	     { "ELSEIF",NULL,&elseif_statement,FALSE},\
	     { "ENDIF",NULL,&endif_statement,FALSE},\
	     { "FOR","NEXT",&for_statement,TRUE},\
	     { "WHILE","WEND",&while_statement,TRUE},\
	     { "WEND",NULL,&wend_statement,FALSE},\
	     { "PRINT",NULL,&print_statement,FALSE},\
	     { "IMPORT",NULL,&import_statement,FALSE},\ 
	     { "END",NULL,&end_statement,FALSE},\ 
	     { "FUNCTION","ENDFUNCTION",&function_statement,TRUE},\ 
	     { "ENDFUNCTION",NULL,&endfunction_statement,FALSE},\ 
	     { "RETURN",NULL,&return_statement,FALSE},\ 
	     { "DECLARE",NULL,&declare_statement,FALSE},\
	     { "ITERATE",NULL,&iterate_statement,FALSE},\
	     { "NEXT",NULL,&next_statement,FALSE},\
	     { "EXIT",NULL,&exit_statement,FALSE},\
	     { "TYPE","ENDTYPE",&type_statement,FALSE},\
	     { "TRY","ENDTRY",&try_statement,FALSE},\
	     { "ENDTRY",NULL,&endtry_statement,FALSE},\
	     { "CATCH",NULL,&catch_statement,FALSE},\
	     { "RESIZE",NULL,&resize_statement,FALSE},\
	     { "AS",NULL,&bad_keyword_as_statement,FALSE},\
	     { "TO",NULL,&bad_keyword_as_statement,FALSE},\
	     { "STEP",NULL,&bad_keyword_as_statement,FALSE},\
	     { "THEN",NULL,&bad_keyword_as_statement,FALSE},\
	     { "AS",NULL,&bad_keyword_as_statement,FALSE},\
	     { "DOUBLE",NULL,&bad_keyword_as_statement,FALSE},\
	     { "STRING",NULL,&bad_keyword_as_statement,FALSE},\
	     { "INTEGER",NULL,&bad_keyword_as_statement,FALSE},\
	     { "SINGLE",NULL,&bad_keyword_as_statement,FALSE},\
	     { "LONG",NULL,&bad_keyword_as_statement,FALSE},\
	     { "AND",NULL,&bad_keyword_as_statement,FALSE},\
	     { "OR",NULL,&bad_keyword_as_statement,FALSE},\
	     { "NOT",NULL,&bad_keyword_as_statement,FALSE},\
	     { "ENDTYPE",NULL,&bad_keyword_as_statement,FALSE},\
	     { "ERR",NULL,&bad_keyword_as_statement,FALSE},\
	     { "ERRL",NULL,&bad_keyword_as_statement,FALSE},\
	     { "ERRFUNC",NULL,&bad_keyword_as_statement,FALSE},\
	     { "QUIT",NULL,&quit_command,FALSE},\
	     { "VARIABLES",NULL,&variables_command,FALSE},\
	     { "CONTINUE",NULL,&continue_command,FALSE},\
	     { "LOAD",NULL,&load_command,FALSE},\
	     { "RUN",NULL,&run_command,FALSE},\
	     { "SET",NULL,&set_command,FALSE},\
	     { "CLEAR",NULL,&clear_command,FALSE},\
	     { "HELP",NULL,&help_command,FALSE},\
	     { "TRACE",NULL,&trace_command,FALSE},\
	     { "STACKTRACE",NULL,&stacktrace_command,FALSE},\
	     { NULL,NULL,NULL } };

int CallIfStatement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int statementcount=0;
char *statement[MAX_SIZE];

/* search through struct for statement */

do {
	if(statements[statementcount].statement == NULL) break;

	if(strcmpi(statements[statementcount].statement,tokens[0]) == 0) {  	/* found statement */
		return(statements[statementcount].call_statement(tc,tokens)); 		/* call statement */
	}
	
	statementcount++;

} while(statements[statementcount].statement != NULL);

return(INVALID_STATEMENT);
}

int IsStatement(char *statement) {
int statementcount=0;

do {
	if(statements[statementcount].statement == NULL) return(FALSE);

	if(strcmpi(statements[statementcount].statement,statement) == 0) return(TRUE);

} while(statements[statementcount++].statement != NULL);

return(FALSE);
}

int IsEndStatementForStatement(char *statement,char *endstatement) {
int statementcount=0;

do {
	if(statements[statementcount].statement == NULL) break;

	if(strcmpi(statements[statementcount].statement,statement) == 0) {	/* found statement */
		if(strcmpi(statements[statementcount].endstatement,endstatement) == 0) return(TRUE); /* found statement */
	}

} while(statements[statementcount++].statement != NULL);

return(FALSE);
}

int IsBlockStatement(char *statement) {
int statementcount=0;

do {
	if(statements[statementcount].statement == NULL) break;

	if(strcmpi(statements[statementcount].statement,statement) == 0) {	/* found statement */
		if(statements[statementcount].endstatement != NULL) return(TRUE); /* found statement */
	}

} while(statements[statementcount++].statement != NULL);

return(FALSE);
}
