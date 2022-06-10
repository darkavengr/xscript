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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"

/*
 * Evaluate expression
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
       int end				End in array
	
 * Returns error message -1 on error or result on success
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

SubstituteVariables(start,end,temp);

/* do syntax checking on expression */

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"(") == 0) bracketcount++;		/* open bracket */
 if(strcmp(temp[count],")") == 0) bracketcount--;		/* close bracket */


}

if(bracketcount != -1) {				/* mismatched brackets */
 PrintError(SYNTAX_ERROR);
 return(-1);
}

for(count=start;count<end;count++) {
 if((GetVariableType(temp[count]) == VAR_STRING) && (GetVariableType(temp[count+1]) != VAR_STRING)) {
  PrintError(TYPE_ERROR);
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
  sprintf(temp[count-1],"%d",EvaluateCondition(temp,count-1,count+2));
  
//  DeleteFromArray(temp,count+1,2);		/* remove rest */
 } 

}

val.d = atof(temp[start]);

// BIDMAS
for(count=start;count<end;count++) {

 if(strcmp(temp[count],"/") == 0) {
  val.d /= atof(temp[count+1]);
  DeleteFromArray(temp,count,2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"*") == 0) {

  val.d *= atof(temp[count+1]);

  DeleteFromArray(temp,count,2);		/* remove rest */
  count++;
	
 } 
}
     
for(count=start;count<end;count++) {

 if(strcmp(temp[count],"+") == 0) { 

  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"-") == 0) {

  val.d -= atof(temp[count+1]);
  DeleteFromArray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"!") == 0) {

  val.d += !atof(temp[count+1]);
 
  DeleteFromArray(temp,count,2);		/* remove rest */
 } 

   count++;
}


for(count=start;count<end;count++) {
 if(strcmp(temp[count],"&") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"|") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"^") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,2);		/* remove rest */
 } 

 count++;
}

printf("return=%.6g\n",val.d);
return(val.d);
}

int DeleteFromArray(char *arr[255][255],int n,int end) {
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

int EvaluateCondition(char *tokens[][MAX_SIZE],int start,int end) {
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
	  if((GetVariableType(tokens[exprpos-1]) != -1) && (GetVariableType(tokens[exprpos-1]) != -1)) {
		if( GetVariableType(tokens[exprpos-1]) != GetVariableType(tokens[exprpos+1])) {
		   PrintError(TYPE_ERROR);
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

	if(GetVariableType(tokens[exprpos-1]) == VAR_STRING) {		/* comparing strings */
	 ConatecateStrings(start,exprpos,tokens,&val);					/* join all the strings on the line */
	 ConatecateStrings(exprpos+1,end,tokens,&val);					/* join all the strings on the line */

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
