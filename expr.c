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
#include <math.h>

#include "define.h"
#include "expr.h"

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
char *token;
double exprone;
char *temp[MAX_SIZE][MAX_SIZE];
char *subexpr[MAX_SIZE][MAX_SIZE];
varval val;
int startexpr;
int endexpr;
int exprcount;
int ti;

memset(temp,0,MAX_SIZE*MAX_SIZE);

/* do expressions in brackets */

exprcount=0;

for(count=start;count<end;count++) {
 if(strcmp(tokens[count],"(") == 0) {				/* start of expression */ 
		startexpr=count+1;

		while(count < end) {
		 if(strcmp(tokens[count] ,")") == 0) break;
		 count++;
		}
		
		SubstituteVariables(startexpr,count,tokens,subexpr);
	
		exprone=doexpr(subexpr,startexpr,count);
					
		sprintf(temp[exprcount++],"%.6g",exprone);
			
 }
 else
 {		
		strcpy(temp[exprcount++],tokens[count]);
 }

}

SubstituteVariables(0,exprcount,temp,temp);


for(count=0;count<exprcount;count++) {
 if((GetVariableType(temp[count]) == VAR_STRING) && (GetVariableType(temp[count+1]) != VAR_STRING)) {
  PrintError(TYPE_ERROR);
  return(-1);
 }
}

val.d=atof(temp[0]);

for(count=0;count<exprcount;count++)  {

 if((strcmp(temp[count],"<") == 0) || (strcmp(temp[count],">") == 0) || (strcmp(temp[count],"=") == 0) || (strcmp(temp[count],"!=") == 0)) {
  sprintf(temp[count],"%d",EvaluateCondition(temp,count-1,count+2));

  //DeleteFromArray(temp,count,2);		/* remove rest */    
 } 

}

// BIDMAS
for(count=0;count<exprcount;count++)  {

 if(strcmp(temp[count],"/") == 0) {
  val.d /= atof(temp[count+1]);
  //DeleteFromArray(temp,count,count+2);		/* remove rest */

  count++;
 } 
}

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"*") == 0) { 
  val.d *= atof(temp[count+1]);

  //DeleteFromArray(temp,count,count+2);		/* remove rest */
//  count++;
	
 } 
}

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"+") == 0) { 

  val.d += atof(temp[count+1]);

//  //DeleteFromArray(temp,count,count+2);		/* remove rest */

  count++;
//  continue;

 } 
}

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"-") == 0) {

  val.d -= atof(temp[count+1]);
  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}


/* power */

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"**") == 0) {

  val.d += pow(atof(temp[count-1]),atof(temp[count+1]));
  DeleteFromArray(temp,count,count+3);		/* remove rest */
 } 

  count++;
}



for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"%") == 0) {
  ti=val.d;

  ti %= (int) atoi(temp[count+1]);

  val.d=(double) ti;

  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}
/* bitwise not */

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"~") == 0) {

  val.d += !atof(temp[count+1]);
 
  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

   count++;
}

/* bitwise and */

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"&") == 0) {
  val.d += atof(temp[count+1]);

  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"|") == 0) {
  val.d += atof(temp[count+1]);

  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

  count++;
}

for(count=0;count<exprcount;count++)  {
 if(strcmp(temp[count],"^") == 0) {
  val.d += atof(temp[count+1]);

  //DeleteFromArray(temp,count,count+2);		/* remove rest */
 } 

 count++;
}

return(val.d);
}

int DeleteFromArray(char *arr[MAX_SIZE][MAX_SIZE],int start,int end,int deletestart,int deleteend) {
 int count;
 char *temp[10][255];
 int oc=0;

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
 * Evalue a single condition
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
       int end				End in array

 * Returns true or false
 *
 */

int EvaluateSingleCondition(char *tokens[][MAX_SIZE],int start,int end) {
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

	exprone=doexpr(tokens,start,exprpos-1);				/* do expression */
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

int EvaluateCondition(char *tokens[][MAX_SIZE],int start,int end) {
int startcount;
int endcount;
int resultcount=0;
int count;
int overallresult=0;
int countx;

struct {
 int result;
 int and_or;
} results[MAX_SIZE];

//456 > 123 and 124 > 666 and 999 > 888
//456 > 123
//124 > 666
//999 > 888

// Evaluate sub-conditions

startcount=1;

count=start;

while(count < end) {
  if(strcmpi(tokens[count],"THEN") == 0) break;
  
  if((strcmpi(tokens[count],"AND") == 0) || (strcmpi(tokens[count],"OR") == 0)) {
	  if(strcmpi(tokens[count],"AND") == 0) { 
		results[resultcount].result=EvaluateSingleCondition(tokens,startcount,count);
		results[resultcount++].and_or=CONDITION_AND;
	  }


	startcount=(count+1);
	count++;
  }
  else
  {
	count++;
  }
}

// If there are no sub conditions, use whole expression

if(resultcount == 0) return(EvaluateSingleCondition(tokens,start,end));

count=0;
overallresult=0;

while(count < resultcount) {
 if(results[count].and_or == CONDITION_AND) {		// and

   overallresult = (results[count].result == results[count+1].result);
   count += 2;
 }
 else if(results[count].and_or == CONDITION_OR) {		// or
   overallresult = (results[count].result || results[count+1].result);
   count += 2;
  }
 }

 return(overallresult);
}

