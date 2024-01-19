
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
#define LINE_SIZE 98
#define INITIAL_BUFFERSIZE 1024
#define MAX_INCLUDE 10
#define MAX_SIZE 256

#define TRUE 1
#define FALSE 0

#define INTERACTIVE_MODE_FLAG	1
#define IS_RUNNING_FLAG		2
#define IS_FILE_LOADED_FLAG	4

#define NO_ERROR	    0
#define FILE_NOT_FOUND	    1
#define NO_PARAMS	    2
#define BAD_EXPRESSION	    3
#define IF_NO_ENDIF	    4
#define FOR_NO_NEXT	    5
#define WHILE_WITHOUT_WEND  6
#define ELSE_WITHOUT_IF     7
#define ENDIF_NOIF          8
#define ENDFUNCTION_NO_FUNCTION 9
#define BAD_VARNAME	    10
#define NO_MEM		    11
#define EXIT_FOR_WITHOUT_FOR 12
#define READ_ERROR	    13
#define SYNTAX_ERROR	    14
#define MISSING_LIBSYM	    15
#define INVALID_STATEMENT   16
#define NESTED_FUNCTION	    17
#define FUNCTION_NO_ENDFUNCTION 18
#define NEXT_WITHOUT_FOR    19
#define WEND_NOWHILE	    20
#define FUNCTION_IN_USE	    21
#define TOO_FEW_ARGS	    22
#define BAD_ARRAY	    23
#define TYPE_ERROR	    24
#define BAD_TYPE	    25
#define CONTINUE_NO_LOOP    26
#define ELSEIF_NOIF	    27
#define BAD_CONDITION	    28
#define BAD_TYPE	    29
#define NO_MODULE_PATH 	    30
#define VARIABLE_EXISTS	    31
#define VARIABLE_DOES_NOT_EXIST 32
#define EXIT_WHILE_WITHOUT_WHILE 33
#define FOR_WITHOUT_NEXT   34
#define TYPE_EXISTS	   35
#define TYPE_NO_ENDTYPE	   36
#define TYPE_FIELD_DOES_NOT_EXIST 37

#define FOR_STATEMENT 1
#define IF_STATEMENT 2
#define WHILE_STATEMENT 4
#define FUNCTION_STATEMENT 8

#define EQUAL 1
#define NOTEQUAL 2
#define GTHAN 3
#define LTHAN 4
#define EQLTHAN 5
#define EQGTHAN 6

#define DEFAULT_SIZE 1024

#define VAR_NUMBER  0
#define VAR_STRING  1
#define VAR_INTEGER 2
#define VAR_SINGLE  3
#define VAR_UDT     4

#define MAX_NEST_COUNT 256

#define ARRAY_SUBSCRIPT 1
#define ARRAY_SLICE	2

#define SUBST_VAR	1
#define SUBST_FUNCTION	2

#define INTERACTIVE_BUFFER_SIZE 65536

#define XSCRIPT_VERSION_MAJOR 1
#define XSCRIPT_VERSION_MINOR 0

#include <stdio.h>

typedef struct {
 double d;
 char *s;
 int i;
 float f;
 int type;
} varval;

typedef struct {
 char *fieldname[MAX_SIZE];
 varval *fieldval;
 int xsize;
 int ysize;
 int type;
 struct UserDefinedTypeField *next;
} UserDefinedTypeField;

typedef struct  {
 char *name[MAX_SIZE];
 UserDefinedTypeField *field;
 struct UserDefinedType *next;
} UserDefinedType;

typedef struct {
 char *varname[MAX_SIZE];
 varval *val;
 UserDefinedType *udt;
 int xsize;
 int ysize;
 char *type[MAX_SIZE];
 int type_int;
 struct vars_t *next;
} vars_t;

typedef struct {
 char *name[MAX_SIZE];
 int x;
 int y;
 int arraytype;
 char *fieldname[MAX_SIZE];
 int fieldx;
 int fieldy;
} varsplit;

typedef struct {
 char *bufptr;
 int lc;
 struct SAVEINFORMATION *last;
 struct SAVEINFORMATION *next;
} SAVEINFORMATION;

typedef struct {
 char *name[MAX_SIZE];
 char *fieldname[MAX_SIZE];
 char *funcstart;
 int funcargcount;
 char *returntype[MAX_SIZE];
 int type_int;
 vars_t *parameters;
 int lc;
 struct functions *next;
} functions;

typedef struct {
 char *name[MAX_SIZE];
 char *callptr;
 int lc;
 SAVEINFORMATION *saveinformation;
 SAVEINFORMATION *saveinformation_top;
 vars_t *vars;
 int stat;	
 char *returntype[MAX_SIZE];
 int type_int;
 int lastlooptype;
 struct FUNCTIONCALLSTACK *next;
 struct FUNCTIONCALLSTACK *last;
} FUNCTIONCALLSTACK;

typedef struct {
 char *statement;
 char *endstatement;
 unsigned int (*call_statement)(int,void *);		/* function pointer */
 int is_block_statement;
} statement;

typedef struct {
 char *modulename[MAX_SIZE];
 void  (*dladdr)(void);			/* function pointer */
 void *dlhandle;
 struct MODULES *next;
} MODULES;

typedef struct {
 int linenumber;
 char *functionname[MAX_SIZE];
 struct BREAKPOINT *next;
} BREAKPOINT;

typedef struct {
 varval val;
 UserDefinedType *udt;
} functionreturnvalue;

