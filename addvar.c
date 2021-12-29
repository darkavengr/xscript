#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"
#include "defs.h"

int init_funcs(void);
int free_funcs(void);
int addvar(char *name,int type,int xsize,int ysize);
int updatevar(char *name,varval *val,int x,int y);
int resizearray(char *name,int x,int y);
int getvarval(char *name,varval *val);
int getvartype(char *name);
int splitvarname(char *name,varsplit *split);
int removevar(char *name);
int function(char *name,char *args,int function_return_type);
int check_function(char *name);
int atoi_base(char *hex,int base);
int substitute_vars(int start,int end,char *tokens[][MAX_SIZE]);

extern varval retval;
functions *funcs=NULL;
functions *currentfunction=NULL;

char *vartypenames[] = { "DOUBLE","STRING","INTEGER","SINGLE",NULL };

extern char *currentptr;
extern statement statements[];

int returnvalue=0;

struct {
 char *callptr;
 functions *funcptr;
 int lc;
} callstack[MAX_NEST_COUNT];

int callpos=0;

int init_funcs(void) {
funcs=malloc(sizeof(funcs));		/* functions */
if(funcs == NULL) {
 print_error(NO_MEM);
 return(-1);
}

memset(funcs,0,sizeof(funcs));

currentfunction=funcs;

callstack[0].funcptr=funcs;
strcpy(currentfunction->name,"main");		/* set function name */
}

int free_funcs(void) {
 free(funcs);
}

int addvar(char *name,int type,int xsize,int ysize) {
vars_t *next;
vars_t *last;
void *o;
char *n;
int *resize;
char *arr[MAX_SIZE];
int arrval;
varsplit split;
functions *funcnext;
int statementcount;

splitvarname(name,&split);				/* parse variable name */

touppercase(split.name);

statementcount=0;
 
do {
 if(statements[statementcount].statement == NULL) break;

 if(strcmp(statements[statementcount].statement,split.name) == 0) return(BAD_VARNAME);
 
 statementcount++;

} while(statements[statementcount].statement != NULL);

 if(currentfunction->vars == NULL) {			/* first entry */
  currentfunction->vars=malloc(sizeof(vars_t));		/* add new item to list */
  if(currentfunction->vars == NULL) return(NO_MEM);	/* can't resize */

  next=currentfunction->vars;
 }
 else
 {
  next=currentfunction->vars;						/* point to variables */

  while(next != NULL) {
   last=next;
   next=next->next;
  }

  last->next=malloc(sizeof(vars_t));		/* add new item to list */
  if(last->next == NULL) return(NO_MEM);	/* can't resize */

  next=last->next;
 }

 next->val=malloc(sizeof(varval)*(xsize*ysize));
 if(next->val == NULL) {
  free(next);
  return(NO_MEM);
 }
  
 next->xsize=xsize;				/* set size */
 next->ysize=ysize;
 next->type=type;

 strcpy(next->varname,split.name);		/* set name */
 next->next=NULL;

 return;
}
	
int updatevar(char *name,varval *val,int x,int y) {
vars_t *next;
char *o;
varsplit split;
varval *varv;

splitvarname(name,&split);

touppercase(split.name);
next=currentfunction->vars;

 while(next != NULL) {
   if(strcmp(next->varname,split.name) == 0) {		/* already defined */

    if((x*y) > (next->xsize*next->ysize)) {		/* outside array */
	print_error(BAD_ARRAY);
	return;
    }

    switch(next->type) {
     case VAR_NUMBER:			
       next->val[x*y].d=val->d;
       break;

     case VAR_STRING:				/* string */
       strcpy(next->val[x*y].s,val->s);
       break;

     case VAR_INTEGER:	 
       next->val[x*y].i=val->i;
       break;

     case VAR_SINGLE:	     
       next->val[x*y].f=val->f;
       break;

    }
 
    return(0);
   }

  next=next->next;
 }

 return(-1);
}
								
int resizearray(char *name,int x,int y) {
vars_t *next;
char *o;
varsplit split;
int statementcount;
 
splitvarname(name,&split);				/* parse variable name */

touppercase(split.name);

 next=currentfunction->vars;

 while(next != NULL) {
   touppercase(next->varname);
   touppercase(split.name);

   if(strcmp(next->varname,split.name) == 0) {		/* found variable */
    if(realloc(next->val,(x*y)*sizeof(varval)) == NULL) return(-1);	/* resize buffer */
   
    next->xsize=x;
    next->ysize=y;
    return(0);
  }

  next=next->next;
 }

 return(-1);
}

int getvarval(char *name,varval *val) {
vars_t *next;
varsplit split;
char *token;
char c;
char *subscriptptr;
int intval;
double floatval;

if(name == NULL) return(-1);
	
c=*name;

if(c >= '0' && c <= '9') {
 val->d=atof(name);
 return;
}

if(c == '"') {
 strcpy(val->s,name);
 return(0);
}

splitvarname(name,&split);

next=currentfunction->vars;

while(next != NULL) {  

 touppercase(next->varname);		/* to lowercase */
 touppercase(split.name);

 if(strcmp(next->varname,split.name) == 0) { 

   switch(next->type) {
      case VAR_NUMBER:
        val->d=next->val[split.x*split.y].d;
        return(0);

       case VAR_STRING:			 	/* string */
        strcpy(val->s,next->val[split.x*split.y].s);
        return(0);

       case VAR_INTEGER:	     
        val->i=next->val[split.x*split.y].i;
	return(0);

       case VAR_SINGLE:	     
        next->val[split.x*split.y].f=val->f;
	return(0);

       default:
        val->i=next->val[split.x*split.y].i;
        return(0);
    }

  }
   
  next=next->next;
 } 

return(-1);
}

int getvartype(char *name) {
vars_t *next;
varsplit split;

if(name == NULL) return(-1);

if(*name >= '0' && *name <= '9') return(VAR_NUMBER);

if(*name == '"') return(VAR_STRING);

touppercase(name);		/* to lowercase */
splitvarname(name,&split);
 
next=currentfunction->vars;

while(next != NULL) {  

 touppercase(next->varname);		/* to lowercase */
  
 if(strcmp(next->varname,split.name) == 0) {
  return(next->type);
 }

 next=next->next;
}

return(-1);
}

int splitvarname(char *name,varsplit *split) {
char *arrpos;
char *arrx[MAX_SIZE];
char *arry[MAX_SIZE];
char *o;
char *commapos;
char *b;
char *tokens[10][MAX_SIZE];
int tc;

memset(arrx,0,MAX_SIZE);			/* clear buffer */
memset(arry,0,MAX_SIZE);			/* clear buffer */

memset(split,0,sizeof(varsplit)-1);

if((strpbrk(name,"(") == NULL) && (strpbrk(name,"[") == NULL)) {				/* find start of subscript */
 strcpy(split->name,name);
 return;
}

o=name;				/* copy name */
b=split->name;

while(*o != 0) {
 if(*o == '(') {
	split->arraytype=ARRAY_SUBSCRIPT;
	break;
 }

 if(*o == '[') {
	split->arraytype=ARRAY_SLICE;
	break;
 }

 *b++=*o++;
}

commapos=strpbrk(name,",");	/* find comma position */
if(commapos == NULL) {			/* 2d array */
 o++;				/* copy subscript */
 b=arrx;

 while(*o != 0) {
  if(*o == ']' || o == ')') break;
  *b++=*o++;
 }

 tc=tokenize_line(arrx,tokens," ");			/* tokenize line */

 split->x=doexpr(tokens,0,tc);		/* get x pos */
 split->y=1;

 return;
} 
else
{				/* 3d array */

 o++;				/* copy subscript */
 b=arrx;

 while(*o != 0) {
  if(*o == ',') break;
  *b++=*o++;
 }

o++;
 b=arry;

 while(*o != 0) {
  if(*o == ']' || o == ')') break;
  *b++=*o++;
 }

 tc=tokenize_line(arrx,tokens," ");			/* tokenize line */
 split->x=doexpr(tokens,0,tc);		/* get x pos */

 tc=tokenize_line(arry,tokens," ");			/* tokenize line */
 split->y=doexpr(tokens,0,tc);		/* get x pos */
 return;
 }
}

/* remove variable */

int removevar(char *name) {
 vars_t *next;
 vars_t *last;
 varsplit split;

 splitvarname(name,&split);				/* parse variable name */

 touppercase(name);		/* to lowercase */
 
 next=currentfunction->vars;						/* point to variables */
 
 while(next != NULL) {
   last=next;
  
   if(strcmp(next->varname,split.name) == 0) {			/* found variable */
     last->next=next->next;				/* point over link */

//    free(next);
    return;    
   }

  next=next->next;
 }
}

/*
 * declare function
 *
 */


int function(char *name,char *args,int function_return_type) {
 functions *next;
 functions *last;
 char *linebuf[MAX_SIZE];
 char *savepos;
 char *tokens[10][MAX_SIZE];
 int count;
 
 if((currentfunction->stat & FUNCTION_STATEMENT) == FUNCTION_STATEMENT) return(NESTED_FUNCTION);

 touppercase(name);		/* to lowercase */

 next=funcs;						/* point to variables */
 
 while(next != NULL) {
  last=next;
  touppercase(next->name);	/* to lowercase */

  if(strcmp(next->name,name) == 0) return(FUNCTION_IN_USE);	/* already defined */

  next=next->next;
 }

 last->next=malloc(sizeof(functions));		/* add new item to list */
 if(last->next == NULL) return(NO_MEM);		/* can't resize */

 next=last->next;

 strcpy(next->name,name);				/* copy name */
 next->funcargcount=tokenize_line(args,next->argvars,",");			/* copy args */

 next->vars=NULL;
 next->funcstart=currentptr;
 next->return_type=function_return_type;

/* find end of function */

 do {
  currentptr=readlinefrombuffer(currentptr,linebuf,LINE_SIZE);			/* get data */
  tokenize_line(linebuf,tokens," ");			/* copy args */

  touppercase(tokens[0]);

  if(strcmp(tokens[0],"ENDFUNCTION") == 0) break;  
}    while(*currentptr != 0); 			/* until end */

 return;
}

int check_function(char *name) {
functions *next;

next=funcs;						/* point to variables */

/* find function name */

touppercase(name);

while(next != NULL) {
 touppercase(next->name);
 if(strcmp(next->name,name) == 0) return(0);		/* found name */   

 next=next->next;
}
return(-1);
}

/*
 * call function
 *
 */

double callfunc(char *name,char *args) {
functions *next;
char *argbuf[10][MAX_SIZE];
char *varbuf[10][MAX_SIZE];
char *parttokens[10][MAX_SIZE];
int count;
int tc;
char *buf[MAX_SIZE];
varsplit split;
vars_t *vars;
char c;
varval val;
int parttc;
int typecount;
int countx;

next=funcs;						/* point to variables */

/* find function name */

touppercase(name);		/* to lowercase */

while(next != NULL) {
 touppercase(next->name);		/* to lowercase */

 if(strcmp(next->name,name) == 0) break;		/* found name */   

 next=next->next;
}

if(next == NULL) return(INVALID_STATEMENT);

next->vars=NULL;		/* no vars to begin with */

callstack[callpos].callptr=currentptr;	/* save information aboutn the calling function */
callstack[callpos].funcptr=currentfunction;
callpos++;

callstack[callpos].callptr=next->funcstart;	/* nformation about the current function */
callstack[callpos].funcptr=next;

currentfunction=next;
currentptr=next->funcstart;

currentfunction->stat |= FUNCTION_STATEMENT;

//if(tc < next->funcargcount) return(TOO_FEW_ARGS);	/* too few arguments */

/* add variables from args */

tc=tokenize_line(args,varbuf,",");
for(count=0;count < tc;count++) {
  parttc=tokenize_line(next->argvars[count],parttokens," ");		/* split token again */

  /* check if declaring variable with type */
  touppercase(parttokens[1]);

  if(strcmp(parttokens[1],"AS") == 0) {		/* variable type */
 	  typecount=0;

	 touppercase(parttokens[2]);

 	 while(vartypenames[typecount] != NULL) {
	   if(strcmp(vartypenames[typecount],parttokens[2]) == 0) break;	/* found type */
  
	   typecount++;
	 }

	 if(vartypenames[typecount] == NULL) {		/* invalid type */
	   print_error(BAD_TYPE);
	   return(-1);
	}
 }
 else
 {
	typecount=0;
 }


  splitvarname(parttokens[0],&split);		/* get name and subscripts */

  switch(typecount) {
    case VAR_NUMBER:				/* number */
	val.d=atof(varbuf[count]);
	break;

    case VAR_STRING:				/* string */
	strcpy(val.s,varbuf[count]);
	break;

    case VAR_INTEGER:				/* integer */
	val.i=atoi(varbuf[count]);
	break;

    case VAR_SINGLE:				/* single */
	val.f=atof(next->argvars[count]);
	break;
  }

   addvar(split.name,typecount,split.x,split.y);
   updatevar(split.name,&val,split.x,split.y);   
}

/* do function */
	
while(*currentptr != 0) {	
 currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

 tc=tokenize_line(buf,argbuf," \009"); 

 touppercase(argbuf[0]);

 if(strcmp(argbuf[0],"ENDFUNCTION") == 0) break;

 doline(buf);

}

currentptr=readlinefrombuffer(currentptr,buf,LINE_SIZE);			/* get data */

currentfunction->stat &= FUNCTION_STATEMENT;

count=0;

/* remove variables */


vars=currentfunction->vars;

while(vars != NULL) {
  removevar(vars->varname);
  vars=vars->next;
}

return_from_function();			/* return */
return;
}

int return_from_function(void) {
callpos--;
currentfunction=callstack[callpos].funcptr;
currentptr=callstack[callpos].callptr;	/* restore information aboutn the calling function */
}

int atoi_base(char *hex,int base) {
int num=0;
char *b;
char c;
int count=0;
int shiftamount=0;

if(base == 10) shiftamount=1;	/* for decimal */

b=hex;
count=strlen(hex);		/* point to end */

b=b+(count-1);

while(count > 0) {
 c=*b;
 
 if(base == 16) {
  if(c >= 'A' && c <= 'F') num += (((int) c-'A')+10) << shiftamount;
  if(c >= 'a' && c <= 'f') num += (((int) c-'a')+10) << shiftamount;
  if(c >= '0' && c <= '9') num += ((int) c-'0') << shiftamount;

  shiftamount += 4;
  count--;
 }

 if(base == 8) {
  if(c >= '0' && c <= '7') num += ((int) c-'0') << shiftamount;

  shiftamount += 3;
  count--;
 }

 if(base == 2) {
  if(c >= '0' && c <= '1') num += ((int) c-'0') << shiftamount;

  shiftamount += 1;
  count--;
 }

 if(base == 10) {
  num += (((int) c-'0')*shiftamount);

  shiftamount =shiftamount*10;
  count--;
 }

 b--;
}

return(num);
}


int substitute_vars(int start,int end,char *tokens[][MAX_SIZE]) {
int count;
varsplit split;
char *valptr;
char *buf[MAX_SIZE];
functions *next;
char *functionargs[MAX_SIZE];
functions *func;
char *b;
char *d;
int county;
double ret;
varval val;

/* replace non-decimal numbers with decimal equivalents */
 for(count=start;count<end;count++) {
   if(memcmp(tokens[count],"0x",2) == 0 ) {	/* hex number */  
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,16),buf);
    strcpy(tokens[count],buf);
   }

   if(memcmp(tokens[count],"&",1) == 0) {				/* octal number */
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,8),buf);
    strcpy(tokens[count],buf);
   }

   if(memcmp(tokens[count],"0b",2) == 0) {					/* binary number */
    valptr=tokens[count];
    valptr=valptr+2;

    itoa(atoi_base(valptr,2),tokens[count]);
   }
 }

/* replace variables with values */

for(count=start;count<end;count++) { 
 splitvarname(tokens[count],&split);

 if(check_function(split.name) == 0) {			/* is function */
  b=tokens[count];
  d=buf;

/* get function name */

  while(*b != 0) {
   if(*b == '(') break;
   *d++=*b++;
  }

  d=functionargs;
  b++;

/* get function args */

  while(*b != 0) {
   *d++=*b++;
  }

  callfunc(buf,functionargs);

  if(retval.type == VAR_STRING) {		/* returning string */   
   strcpy(tokens[count],retval.s);
   return;
  }
  else if(retval.type == VAR_INTEGER) {		/* returning integer */
	 sprintf(tokens[count],"%d",retval.i);
  }
  else if(retval.type == VAR_NUMBER) {		/* returning double */
	 sprintf(tokens[count],"%.6g",retval.d);
  }
  else if(retval.type == VAR_SINGLE) {		/* returning single */
	 sprintf(tokens[count],"%f",retval.f);
  }

 }

 if(getvarval(split.name,&val) != -1) {		/* is variable */

   switch(getvartype(tokens[count])) {
	case VAR_STRING:
	    if(split.arraytype == ARRAY_SLICE) {		/* part of string */
		b=&val.s;			/* get start */
		b += split.x;

		memset(tokens[count],0,MAX_SIZE);
		d=tokens[count];
		*d++='"';

		for(count=0;count < split.y+1;count++) {
		 *d++=*b++;
		}

		break;
	    }
	    else
	    {
	     d=tokens[count];
	     *d++='"';
		
	     strcpy(tokens[count],val.s);
            }

	    break;
	
	case VAR_NUMBER:		   
	    sprintf(tokens[count],"%.6g",val.d);          
   	    break;

	case VAR_INTEGER:	
	    sprintf(tokens[count],"%d",val.i);
	    break;

       case VAR_SINGLE:	     
	    sprintf(tokens[count],"%f",val.f);
            break;
	}
	
     }
  }
}

int conatecate_strings(int start,int end,char *tokens[][MAX_SIZE],varval *val) {
int count;
char *b;
char *d;

substitute_vars(start,end,tokens);

b=tokens[start];			/* point to first token */
if(*b == '"') b++;			/* skip over quote */

d=val->s;				/* copy token */
while(*b != 0) {
 if(*b == '"') break;
 if(*b == 0) break;

 *d++=*b++;
}

for(count=start+1;count<end;count++) {

 if(strcmp(tokens[count],"+") == 0) { 

    b=tokens[count+1];
    if(*b == '"') b++;			/* skip over quote */

    while(*b != 0) {			/* copy token */
     if(*b == '"') break;
     if(*b == 0) break;

     *d++=*b++;
    }

    count++;
   }
  }

 return;
}


