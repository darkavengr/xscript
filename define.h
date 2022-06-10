
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

#define NO_ERROR	    0
#define FILE_NOT_FOUND	    1
#define NO_PARAMS	    2
#define BAD_EXPRESSION	    3
#define IF_NO_ENDIF	    4
#define FOR_NO_NEXT	    5
#define WHILE_NO_WEND	    6
#define ELSE_NOIF           7
#define ENDIF_NOIF          8
#define ENDFUNCTION_NO_FUNCTION 9
#define BAD_VARNAME	    10
#define NO_MEM		    11
#define INVALID_BREAK	    12
#define READ_ERROR	    13
#define SYNTAX_ERROR	    14
#define MISSING_LIBSYM	    15
#define INVALID_STATEMENT   16
#define NESTED_FUNCTION	    17
#define FUNCTION_NO_ENDFUNCTION 18
#define NEXT_NO_FOR	    19
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
#define TYPE_EXISTS	    31

#define TRUE 0
#define FALSE 1

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

#define MAX_NEST_COUNT 256

#define ARRAY_SUBSCRIPT 1
#define ARRAY_SLICE	2

#include <stdio.h>

typedef struct {
 double d;
 char *s[MAX_SIZE];
 int i;
 float f;
 int type;
} varval;

typedef struct {
 char *varname[MAX_SIZE];
 varval *val;
 int xsize;
 int ysize;
 int type;
 struct vars_t *next;
} vars_t;
 
typedef struct {
 char *filename[MAX_SIZE];
 FILE *handle;
 int lc;
} include;


typedef struct {
 char *name[MAX_SIZE];
 int x;
 int y;
 int arraytype;
} varsplit;

typedef struct {
 char *bufptr;
 int lc;
} SAVEINFORMATION;

typedef struct {
 char *name[MAX_SIZE];
 char *funcstart;
 int funcargcount;
 int stat;	
 int lastlooptype;
 int nestcount;
 int return_type;
 SAVEINFORMATION saveinformation[MAX_NEST_COUNT];
 struct vars_t *vars;
 char *argvars[25][MAX_SIZE];
 struct functions *next;
} functions;

typedef struct {
 char *statement;
 unsigned int (*call_statement)(int,void *);		/* function pointer */
} statement;

typedef struct {
 char *modulename[MAX_SIZE];
 void  (*dladdr)(void);			/* function pointer */
 void *dlhandle;
 struct MODULES *next;
} MODULES;

