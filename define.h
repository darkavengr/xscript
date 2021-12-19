#define LINE_SIZE 98
#define INITIAL_BUFFERSIZE 1024
#define MAX_INCLUDE 10
#define MAX_SIZE 256

#define NO_ERROR	    0
#define NO_GOTO 	    1
#define MISSING_FILE	    2
#define NO_PARAMS	    3
#define BAD_EXPRESSION	    4
#define IF_NO_ENDIF	    5
#define FOR_NO_NEXT	    6
#define WHILE_NO_WEND	    7
#define ELSE_NOIF           8
#define ENDIF_NOIF          9
#define ENDFUNCTION_NO_FUNCTION 10
#define BAD_VARNAME	    11
#define NO_MEM		    12
#define EXITFOR_NO_FOR	    13
#define READ_ERROR	    14
#define SYNTAX_ERROR	    15
#define MISSING_LIBSYM	    16
#define INVALID_STATEMENT   17
#define NESTED_FUNCTION	    18
#define FUNCTION_NO_ENDFUNCTION 19
#define NEXT_NO_FOR	    20
#define WEND_NOWHILE	    21
#define FUNCTION_IN_USE	    22
#define TOO_FEW_ARGS	    23
#define EXITWHILE_NO_LOOP    24
#define BAD_ARRAY	    25
#define TYPE_ERROR	    26
#define BAD_TYPE	    27
#define CONTINUE_NO_LOOP    28
#define ELSEIF_NOIF	    29

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

#include <stdio.h>

typedef struct {
  double d;
  char *s[MAX_SIZE];
  int i;
  float f;
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
} include;


typedef struct {
 char *name[MAX_SIZE];
 int x;
 int y;
} varsplit;

typedef struct {
 char *bufptr;
 int lc;
} SAVEINFORMATION;

typedef struct {
 char *name[MAX_SIZE];
 char *funcstart;
 int funcargcount;
 int retval;
 int stat;	
 int lastlooptype;
 int nestcount;
 SAVEINFORMATION saveinformation[MAX_NEST_COUNT];
 struct vars_t *vars;
 char *argvars[25][MAX_SIZE];
 struct functions *next;
} functions;

typedef struct {
 char *statement;
 unsigned int (*call_statement)(int,void *);		/* function pointer */
} statement;

