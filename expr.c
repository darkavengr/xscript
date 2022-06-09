#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"
#include "defs.h"


/*
 * Evaluate expression
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
       int end				End in array
	
 * Returns error number on error or result on success
 *
 */
double doexpr(char *tokens[][MAX_SIZE],int start,int end) {
int count;
double x;
char *token;
int countx;
double exprone;
char *temp[end-start][MAX_SIZE];
char *brackettemp[10][MAX_SIZE];
int countz;
int bracketcount=-1;
char *b;
char *d;
varval val;
val.d=0;

memset(temp,0,((end-1)-start)*MAX_SIZE);

/* make a copy */ 
for(count=start;count<end;count++) {
 strcpy(temp[count],tokens[count]);
}

substitute_vars(start,end,temp);

/* do syntax checking on expression */

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"(") == 0) bracketcount++;		/* open bracket */
 if(strcmp(temp[count],")") == 0) bracketcount--;		/* close bracket */


}

if(bracketcount != -1) {				/* mismatched brackets */
 print_error(SYNTAX_ERROR);
 return(-1);
}

for(count=start;count<end;count++) {
 if((getvartype(temp[count]) == VAR_STRING) && (getvartype(temp[count+1]) != VAR_STRING)) {
  print_error(TYPE_ERROR);
  return(-1);
 }
}

if(start+1 == end) {
 return(atof(temp[start]));
}

/* do expressions in brackets */

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"(") == 0) {
  for(countx=count+1;countx<end;countx++) {

   if(strcmp(temp[countx],")") == 0) {

    sprintf(brackettemp[bracketcount],"%.6g",doexpr(temp,count+1,countx-1));		/* do expression */  

    break;
 
   }
  }

  for(bracketcount=count;bracketcount<countx;bracketcount++) {
     strcpy(temp[count],brackettemp[bracketcount]);
  }

 }
 else
 {
     strcpy(brackettemp[bracketcount],temp[count]);

 }
}


for(count=start;count<end;count++) {

 if((strcmp(temp[count],"<") == 0) || (strcmp(temp[count],">") == 0) || (strcmp(temp[count],"=") == 0) || (strcmp(temp[count],"!=") == 0)) {
  sprintf(temp[count-1],"%d",do_condition(temp,count-1,count+2));
  
//  deletefromarray(temp,count+1,2);		/* remove rest */
 } 

}

val.d = atof(temp[start]);

// BIDMAS
for(count=start;count<end;count++) {

 if(strcmp(temp[count],"/") == 0) {
  val.d /= atof(temp[count+1]);
  deletefromarray(temp,count,2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"*") == 0) {

  val.d *= atof(temp[count+1]);

  deletefromarray(temp,count,2);		/* remove rest */
  count++;
	
 } 
}
     
for(count=start;count<end;count++) {

 if(strcmp(temp[count],"+") == 0) { 
  val.d += atof(temp[count+1]);
  deletefromarray(temp,count,2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"-") == 0) {

  val.d -= atof(temp[count+1]);
  deletefromarray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"!") == 0) {

  val.d += !atof(temp[count+1]);
 
  deletefromarray(temp,count,2);		/* remove rest */
 } 

   count++;
}


for(count=start;count<end;count++) {
 if(strcmp(temp[count],"&") == 0) {
  val.d += atof(temp[count+1]);

  deletefromarray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"|") == 0) {
  val.d += atof(temp[count+1]);

  deletefromarray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"^") == 0) {
  val.d += atof(temp[count+1]);

  deletefromarray(temp,count,2);		/* remove rest */
 } 

 count++;
}

return(val.d);
}

int deletefromarray(char *arr[255][255],int n,int end) {
 int count;
 char *temp[10][255];
 int oc=0;

 for(count=n;count<end;count++) {
   strcpy(temp[oc++],arr[count]); 
 }

 for(count=0;count<n;count++) {
   strcpy(arr[count],temp[count]); 
 }

 for(count=n;count<end;count++) { 
  arr[count][0]=NULL;
 }
return;
}

/*
 * Evalue condition
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
       int end				End in array

 * Returns true or false
 *
 */

int do_condition(char *tokens[][MAX_SIZE],int start,int end) {
int inverse;
double exprone;
double exprtwo;
int ifexpr=-1;
int exprtrue=0;
int exprpos=0;
int count=0;
int conditions[MAX_SIZE];
int condcount=0;
varval val;

/* check kind of expression */

 exprone=0;
 exprtwo=0;

 for(exprpos=start;exprpos<end;exprpos++) {
	  if((getvartype(tokens[exprpos-1]) != -1) && (getvartype(tokens[exprpos-1]) != -1)) {
		if( getvartype(tokens[exprpos-1]) != getvartype(tokens[exprpos+1])) {
		   print_error(TYPE_ERROR);
		   return(-1);
        	  }
	  }

	  if(strcmp(tokens[exprpos],"=") == 0) {
	   ifexpr=EQUAL;
	   break;
	  }
	  else if(strcmp(tokens[exprpos],"!=") == 0) {
	   ifexpr=NOTEQUAL;
	   break;
	  }
	  else if(strcmp(tokens[exprpos],">") == 0) {
	   ifexpr=GTHAN;
	   break;	
          }
	  else if(strcmp(tokens[exprpos],"<") == 0) {           
	   ifexpr=LTHAN;
           break;
	  }
	  else if(strcmp(tokens[exprpos],"=<") == 0) {
           ifexpr=EQLTHAN;
           break;
	  }
	  else if(strcmp(tokens[exprpos],">=") == 0) {
           ifexpr=EQGTHAN;
           break;
	  }
 }

	if(ifexpr == -1) return(-1);

	if(getvartype(tokens[exprpos-1]) == VAR_STRING) {		/* comparing strings */
	 conatecate_strings(start,exprpos,tokens,&val);					/* join all the strings on the line */
	 conatecate_strings(exprpos+1,end,tokens,&val);					/* join all the strings on the line */

	 return(!strcmp(tokens[exprpos-1],tokens[exprpos+1]));
	}
	
	exprone=doexpr(tokens,start,exprpos);				/* do expression */
        exprtwo=doexpr(tokens,exprpos+1,end);				/* do expression */

        exprtrue=0;

  	switch(ifexpr) {

          case EQUAL:
           return(exprone == exprtwo);

          case NOTEQUAL:					/* exprone != exprtwo */ 
	   return(exprone != exprtwo);

          case LTHAN:						/* exprone < exprtwo */
	   return(exprone < exprtwo);

          case GTHAN:						/* exprone > exprtwo */
	   return(exprone > exprtwo);

          case EQLTHAN:	           /* exprone =< exprtwo */
	   return(exprone <= exprtwo);

          case EQGTHAN:						/* exprone >= exprtwo */
	   return(exprone >= exprtwo);
          }

	
}
