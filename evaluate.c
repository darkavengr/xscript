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
#include <stdbool.h>
#include "size.h"
#include "variablesandfunctions.h"
#include "evaluate.h"
#include "errors.h"

/*
* Evaluate expression
*
* In: tokens[][MAX_SIZE]	Tokens array containing expression
*     start			Start in array
*     end			End in array
*
* Returns: result of expression
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
int countx;

memset(temp,0,MAX_SIZE*MAX_SIZE);

/* do expressions in brackets */

exprcount=0;

for(count=start;count<end;count++) {
	if((strcmp(tokens[count],"(") == 0)) {				/* start of expression */ 
		if(CheckFunctionExists(tokens[count-1]) == 0) {
			while(count < end) {
				if(strcmp(tokens[count] ,")") == 0) break;
		 		count++;
			}
		
			continue;
		}
		else if(IsVariable(tokens[count-1]) == 0) {
			for(countx=count+1;countx<end;countx++) {
	 		 if(strcmp(tokens[countx] ,")") == 0) break;
		}
		
		continue;
		}
		else
		{
			startexpr=count+1;

			while(count < end) {
		 		if(strcmp(tokens[count] ,")") == 0) break;

		 		count++;
			}
		
			SubstituteVariables(startexpr,count,tokens,subexpr);
	
			exprone=doexpr(subexpr,startexpr,count);
	
			sprintf(temp[exprcount++],"%.6g",exprone);
		}			
	}
	else
	{		
		strcpy(temp[exprcount++],tokens[count]);
	}

}

for(count=0;count<exprcount;count++) {
	if((GetVariableType(temp[count]) == VAR_STRING) && (GetVariableType(temp[count+1]) != VAR_STRING)) {
		PrintError(TYPE_ERROR);
		return(-1);
		}
}

val.d=atof(temp[0]);

for(count=0;count<exprcount;count++)  {

	if((strcmp(temp[count],"<") == 0) || (strcmp(temp[count],">") == 0) || (strcmp(temp[count],"=") == 0) || (strcmp(temp[count],"!=") == 0)) {
	 val.d=EvaluateCondition(temp,count-1,count+2);
	 break;  
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
		count++;
	
	} 
}

for(count=0;count<exprcount;count++)  {
	if(strcmp(temp[count],"+") == 0) { 

		val.d += atof(temp[count+1]);

		//DeleteFromArray(temp,count,count+2);		/* remove rest */

		count++;
	 } 
}

for(count=0;count<exprcount;count++)  {
	if(strcmp(temp[count],"-") == 0) {
		val.d -= atof(temp[count+1]);
	 	//DeleteFromArray(temp,count,count+2);		/* remove rest */
	 	count++;
	 } 
}


/* power */

for(count=0;count<exprcount;count++)  {
	if(strcmp(temp[count],"**") == 0) {

	 val.d += pow(atof(temp[count-1]),atof(temp[count+1]));
	 DeleteFromArray(temp,start,end,count,count+3);		/* remove rest */
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


printf("val.d=%.6g\n",val.d);

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

	* Returns TRUE or FALSE
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
varval firstval,secondval;

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
	 ConatecateStrings(start,exprpos-1,tokens,&firstval);					/* join all the strings on the line */
	 ConatecateStrings(exprpos+1,end,tokens,&secondval);					/* join all the strings on the line */

	 return(!strcmp(firstval.s,secondval.s));
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

int EvaluateCondition(char *tokens[][MAX_SIZE],int start,int end) {
int startcount;
int endcount;
int resultcount=0;
int count;
int overallresult=0;
int countx;
char *evaltokens[MAX_SIZE][MAX_SIZE];
int exprend;
char *temp[MAX_SIZE][MAX_SIZE];
int subcount=0;
int outcount=0;

struct {
	int result;
	int and_or;
} results[MAX_SIZE];

//456 > 123 and 124 > 666 and 999 > 888
//456 > 123
//124 > 666
//999 > 888

// Evaluate sub-conditions

exprend=SubstituteVariables(start,end,tokens,evaltokens);

startcount=start;
count=start;

//
// Do conditions in brackets first
//

while(count < exprend) {
	 if(strcmp(evaltokens[count],"(") == 0) {		// if sub-expression

	  subcount++;
	  startcount=count;

	  while(count < exprend) {
	    if(strcmp(evaltokens[count],")") == 0) {		// end of sub-expression
	      results[resultcount].result=EvaluateSingleCondition(evaltokens,startcount,count+1);

	      resultcount++;

	      if(strcmpi(temp[count],"AND") == 0) results[resultcount].and_or=CONDITION_AND;
	      if(strcmpi(temp[count],"OR") == 0) results[resultcount].and_or=CONDITION_OR;

	      count += subcount;		// Add length of expression
	    }

	    count++;
	  }

	}
	else
	{		
		strcpy(temp[outcount++],evaltokens[count]);
	}

	count++;
}

//
// Do conditions outside brackets
//

startcount=0;

for(count=0;count<outcount;count++) {

	 if((strcmpi(temp[count],"AND") == 0) || (strcmpi(temp[count],"OR") == 0) || (count >= outcount-1)) {
		results[resultcount].result=EvaluateSingleCondition(temp,startcount,count+1);		

		if(strcmpi(temp[count],"AND") == 0) results[resultcount].and_or=CONDITION_AND;
		if(strcmpi(temp[count],"OR") == 0) results[resultcount].and_or=CONDITION_OR;
	
		startcount=(count+1);

		resultcount++;
	}

}

/* if there is more than one result and an odd number of results, set the last to end */

if((resultcount > 1 ) && (resultcount % 2) != 0) {
	results[resultcount-1].and_or=CONDITION_END;
}

// If there are no sub conditions, use whole expression

if(resultcount == 1) return(EvaluateSingleCondition(temp,0,outcount));

overallresult=0;

count=0;

while(count < resultcount) {
	if(results[count].and_or == CONDITION_AND) {		// and
	 overallresult=results[count].result;
	 overallresult=results[count+1].result;

	 count += 2;
	}
	else if(results[count].and_or == CONDITION_OR) {		// or
	 overallresult=(results[count].result || results[count+1].result);
	 count += 2;
	}
	else if(results[count].and_or == CONDITION_END) {		// end
	 if(results[count-1].and_or == CONDITION_AND) {
	  overallresult = (results[count].result == results[count+1].result);
	  count += 2;
	 }
	 else
	 {
	  overallresult = (results[count].result || results[count+1].result);
	  count += 2;
	 }

	}

}

return(overallresult);
}

