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
double doexpr(char *tokens[][MAX_SIZE],int start,int end);

double doexpr(char *tokens[][MAX_SIZE],int start,int end) {
int count;
char *token;
double exprone;
char *temp[MAX_SIZE][MAX_SIZE];
int bracketcount=-1;
varval val;
int countx;
int countz;
int outcount=0;

val.d=0;

/* do expressions in brackets */

for(count=start;count<end;count++) {
 if(strcmp(tokens[count],"(") == 0) {

  for(countx=count;countx<end;countx++) {

   if(strcmp(tokens[countx],")") == 0) {
    val.d = atof(temp[count + 1]);

    exprone=doexpr(tokens,count+2,countx);

    printf("expr=%.6g\n",exprone);
    sprintf(temp[outcount],"%.6g",exprone);		/* do expression */  
    outcount++;

    break; 
   }
  }
 }
 else
 {
  strcpy(temp[outcount],tokens[count]);
  outcount++;
 }

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
  DeleteFromArray(temp,count,count+2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"*") == 0) {

  val.d *= atof(temp[count+1]);

  DeleteFromArray(temp,count,count+2);		/* remove rest */
  count++;
	
 } 
}
     
for(count=start;count<end;count++) {
 if(strcmp(temp[count],"+") == 0) { 

  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,count+2);		/* remove rest */

  count++;
 } 
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"-") == 0) {

  val.d -= atof(temp[count+1]);
  DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"!") == 0) {

  val.d += !atof(temp[count+1]);
 
  DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

   count++;
}


for(count=start;count<end;count++) {
 if(strcmp(temp[count],"&") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"|") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}

for(count=start;count<end;count++) {
 if(strcmp(temp[count],"^") == 0) {
  val.d += atof(temp[count+1]);

  DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

 count++;
}

return(val.d);
}

int DeleteFromArray(char *arr[MAX_SIZE][MAX_SIZE],int start,int end,int deletestart,int deleteend) {
 int count;
 char *temp[10][255];
 int oc=0;

// for(count=start;count<end;count++) {
//    printf("arr[%d]=%s\n",count,arr[count]);
// }

 for(count=start;count<end;count++) {
   if((count < deletestart) || (count > deleteend)) {
    strcpy(temp[oc++],arr[count]); 
   }

 }

 for(count=0;count<oc;count++) {
   strcpy(arr[count],temp[count]); 
 }

 for(count=oc;count<end;count++) {
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
