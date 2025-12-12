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
#include "module.h"
#include "variablesandfunctions.h"
#include "evaluate.h"
#include "errors.h"

/*
* Evaluate expression
*
* In: tokens	Tokens array containing expression
*     start	Start in array
*     end	End in array
*
* Returns: result of expression
*
*/

double EvaluateExpression(char *tokens[][MAX_SIZE],int start,int end) {
int count;
char *token;
double exprone;
char *temp[MAX_SIZE][MAX_SIZE];
char *subexpr[MAX_SIZE][MAX_SIZE];
int startexpr;
int endexpr;
int exprcount;
int ti;
int countx;
int subtc;
char *split_operators[MAX_SIZE][MAX_SIZE];
char *split_operands[MAX_SIZE][MAX_SIZE];
int operatorcount=0;
int operandcount=0;

memset(temp,0,MAX_SIZE*MAX_SIZE);

/* do expressions in brackets */

exprcount=0;

for(count=start;count<end;count++) {
	if((strcmp(tokens[count],"(") == 0)) {				/* start of expression */ 

		if(CheckFunctionExists(tokens[count-1]) == 0) {
			while(count < end) {
				if(strcmp(tokens[count++] ,")") == 0) break;
			}
		
			continue;
		}
		else if(IsVariable(tokens[count-1]) == TRUE) {
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
		
			subtc=SubstituteVariables(startexpr,count+1,tokens,subexpr);
			if(subtc == -1) return(-1);

			exprone=EvaluateExpression(subexpr,0,subtc);

			sprintf(temp[exprcount++],"%.6g",exprone);

		}
	}
	else
	{	
		strncpy(temp[exprcount++],tokens[count],MAX_SIZE);
	}
}

for(count=0;count<exprcount;count++) {
	if((GetVariableType(temp[count]) == VAR_STRING) && (GetVariableType(temp[count+1]) != VAR_STRING)) {
		SetLastError(TYPE_ERROR);
		return(-1);
	}
}

// BIDMAS

/* split operators and operands into two arrays */

operatorcount=0;
operandcount=0;
for(count=0;count < exprcount;count++)  {
	if((strcmp(temp[count],">>") == 0) || (strcmp(temp[count],"<<") == 0) || \
	   (strcmp(temp[count],"+") == 0) || (strcmp(temp[count],"-") == 0) || \
	   (strcmp(temp[count],"*") == 0) || (strcmp(temp[count],"/") == 0) || \
	   (strcmp(temp[count],"~") == 0) || (strcmp(temp[count],"**") == 0) || \
	   (strcmp(temp[count],"%") == 0) || (strcmp(temp[count],"&") == 0) || \
	   (strcmp(temp[count],"|") == 0) || (strcmp(temp[count],"^") == 0)) {
		strncpy(split_operators[operatorcount++],temp[count],MAX_SIZE);
	}
	else
	{
		strncpy(split_operands[operandcount++],temp[count],MAX_SIZE);
	}
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"/") == 0) {
		sprintf(split_operands[count*2],"%.6g",(atof(split_operands[count*2]) / atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"*") == 0) { 
		sprintf(split_operands[count*2],"%.6g",atof(split_operands[count*2]) * atof(split_operands[(count*2)+1]));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	 } 
}
	
for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"+") == 0) {
		sprintf(split_operands[count*2],"%.6g",(atof(split_operands[count*2]) + atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count/2)+1,(count/2)+1);		/* remove second operand */

		count--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"-") == 0) { 
		sprintf(split_operands[count*2],"%.6g",(atof(split_operands[count*2]) - atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"<<") == 0) { 
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) << atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],">>") == 0) { 
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) >> atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	 } 
}

/* power */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"**") == 0) { 
		sprintf(split_operands[count*2],"%.6g",pow(atof(split_operands[count*2]),atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	 } 
}

/* modulus */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"%") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) % atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	}
}

/* bitwise not */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"!") == 0) {
		sprintf(split_operands[count*2],"%d",!atoi(split_operands[count*2]));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */

		count--;
	}
}

/* bitwise and */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"&") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) & atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	}
}

/* bitwise or */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"|") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) | atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	}
}

/* bitwise exclusive or */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"^") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) ^ atoi(split_operands[(count*2)+1])));	

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
	}
}

return(atof(split_operands[0]));
}

void DeleteFromArray(char *arr[MAX_SIZE][MAX_SIZE],int start,int end,int deletestart,int deleteend) {
	int count;
	char *temp[127][MAX_SIZE];
	int oc=0;

	/* copy tokens that are not within range */

	for(count=start;count < end;count++) {
		if((count < deletestart) || (count > deleteend)) {
			strncpy(temp[oc++],arr[count],MAX_SIZE); 
		}

	}

	/* copy the tokens back to the target */

	for(count=0;count < oc;count++) {
		strncpy(arr[count],temp[count],MAX_SIZE); 
	}

	/* clear extra tokens at the end */

	for(count=oc;count < end;count++) {
		arr[count][0]=NULL; 
	}

return;
}


/*
 * Evalue a single condition
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
 *     int end				End in array
 *
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
	else if(strcmp(tokens[exprpos],">=") == 0) {
		ifexpr=EQGTHAN;
		break;	
	}
	else if(strcmp(tokens[exprpos],"<") == 0) {
		ifexpr=LTHAN;
		break;
	}
	else if(strcmp(tokens[exprpos],"<=") == 0) {
		ifexpr=EQLTHAN;
		break;
	}
	else if(strcmp(tokens[exprpos],">") == 0) {
		ifexpr=GTHAN;
		break;
	}
}

if(ifexpr == -1) {	/* evaluate non-zero or zero */

	if(GetVariableType(tokens[start]) == VAR_STRING) {		/* comparing string */
		SetLastError(TYPE_ERROR);
		return(-1);
	}

	return(EvaluateExpression(tokens,start,end));
}

if(GetVariableType(tokens[exprpos-1]) == VAR_STRING) {		/* comparing strings */
	ConatecateStrings(start,exprpos-1,tokens,&firstval);					/* join all the strings on the lines */
	ConatecateStrings(exprpos+1,end,tokens,&secondval);

	return(!strncmp(firstval.s,secondval.s,MAX_SIZE));
}

exprone=EvaluateExpression(tokens,start,exprpos);				/* evaluate expressions */
exprtwo=EvaluateExpression(tokens,exprpos+1,end);

exprtrue=0;

switch(ifexpr) {

	case EQUAL:					/* exprone = exprtwo */
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

/*
 * Evaluate condition
 *
 * In: char *tokens[][MAX_SIZE]		Tokens array containing expression
 *     int start			Start in array
 *     int end				End in array
 *
 * Returns: zero or non-zero
 *
 */

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
int evaltc;
int resultloopcount=0;
int retval;

struct {
	int result;
	int and_or;
} results[MAX_SIZE];

/* Evaluate sub-conditions */

evaltc=SubstituteVariables(start,end,tokens,evaltokens);
if(evaltc == -1) return(-1);

startcount=start;
count=0;

/* Do conditions in brackets first */

for(count=0;count<evaltc+1;count++) {
	if(strcmp(evaltokens[count],"(") == 0) {		/* if sub-expression */
	 	subcount++;
	 	startcount=count;

	 	while(count < exprend) {
	 		if(strcmp(evaltokens[count],")") == 0) {		/* end of sub-expression */
	 			results[resultcount].result=EvaluateSingleCondition(evaltokens,0,count+1);

	 			resultcount++;

	 			if(strcmpi(temp[count],"AND") == 0) results[resultcount].and_or=CONDITION_AND;
	 			if(strcmpi(temp[count],"OR") == 0) results[resultcount].and_or=CONDITION_OR;

	 			count += subcount;		/* Add length of expression */
	    		}

			count++;
	  	}
	}
	else
	{		
		strncpy(temp[outcount++],evaltokens[count],MAX_SIZE);
	}

}

/* Do conditions outside brackets */

	//printf("outcount=%d\n",outcount);

	startcount=0;

	for(count=0;count < outcount;count++) {
		if((strcmpi(temp[count],"AND") == 0) || (strcmpi(temp[count],"OR") == 0) || (count >= outcount-1)) {
			//printf("Found %s condition\n",temp[count]);

			//printf("**********\n");

			//for(countx=startcount;countx < count;countx++) {
			//	printf("temp[%d]=%s\n",countx,temp[countx]);
			//}

			//printf("**********\n");

			results[resultcount].result=EvaluateSingleCondition(temp,startcount,count);		

			//printf("Condition result=%d\n",results[resultcount].result);

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

	/* If there are no sub conditions, use whole expression */

	//printf("resultcount=%d\n",resultcount);

	if(resultcount == 1) {
		retval=EvaluateSingleCondition(temp,0,outcount);

		//printf("retval=%d\n",retval);
		//asm("int $3");

		return(retval);
	}

	overallresult=0;

	resultloopcount=0;

	while(resultloopcount < resultcount) {
		if(results[resultloopcount].and_or == CONDITION_AND) {		// and
			//printf("AND condition\n");
			//printf("AND conditions=%d %d\n",results[resultloopcount].result,results[resultloopcount+1].result);

			overallresult=results[resultloopcount].result && results[resultloopcount+1].result;

			resultloopcount += 2;
		}
		else if(results[resultloopcount].and_or == CONDITION_OR) {		// or
			//printf("OR condition\n");

			overallresult=(results[resultloopcount].result || results[resultloopcount+1].result);
			resultloopcount += 2;
		}
		else if(results[resultloopcount].and_or == CONDITION_END) {		// end
			//printf("END condition\n");

			if(results[resultloopcount-1].and_or == CONDITION_AND) {
				//printf("Previous was AND condition\n");

				overallresult = (overallresult && results[resultloopcount].result);
				resultloopcount += 2;
			}
			else if(results[resultloopcount-1].and_or == CONDITION_OR) {
				//printf("Previous was OR condition\n");

				overallresult = (overallresult || results[resultloopcount].result);
				resultloopcount += 2;
			}
		}
		else
		{
			//printf("other condition\n");

			overallresult = (results[resultloopcount].result || results[resultloopcount+1].result);
		 	resultloopcount += 2;
		}

		//printf("overallresult=%d\n",overallresult);
	}

return(overallresult);
}

/*
 * Check if expression is valid
 *
 * In: tokens				Token array containing expression
 *     int start			Start in array
 *     int end				End in array
 *
 * Returns TRUE or FALSE
 *
 */

int IsValidExpression(char *tokens[][MAX_SIZE],int start,int end) {
int count;
char *ValidExpressionCharacters="+-*/<>=&|!%";
int endexpression;
int IsOperator=FALSE;
int IsValid=TRUE;
int bracketcount=0;
int squarebracketcount=0;
bool IsInBracket=FALSE;
int statementcount=0;
int commacount=0;
int variablenameindex=0;
int starttoken;
int commastart;
char *ConditionCharacters="<>=";

/* check if brackets are balanced */

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"(") == 0) {
		commacount=0;

		if(IsInBracket == FALSE) variablenameindex=(count-1);			/* save name index */

		IsInBracket=TRUE;
		bracketcount++;
	}

	if(strcmp(tokens[count],")") == 0) {
		IsInBracket=FALSE;
		bracketcount--;
	}

	if(strcmp(tokens[count],"[") == 0) {
		IsInBracket=TRUE;
		squarebracketcount++;
	}

	if(strcmp(tokens[count],"]") == 0) {
		IsInBracket=FALSE;
		squarebracketcount--;
	}

	/* check if number of commas is correct for array or function call */
	if(strcmp(tokens[count],",") == 0) {		/* list of expressions */
		commacount++;

		if(IsInBracket == FALSE) return(FALSE);	/* list not in brackets */

		if((bracketcount > 1) || (squarebracketcount > 1)) return(FALSE);

		if(IsVariable(tokens[variablenameindex]) == TRUE) {
			if(commacount >= 2) return(FALSE); /* too many commas for array */
		}
	}
}

if((bracketcount != 0) || (squarebracketcount != 0)) return(FALSE);


/* check if expression is in form {symbol} {op}... */

if(start == end) {	/* kludge */
	if(strpbrk(tokens[start],ValidExpressionCharacters) != NULL) return(FALSE);
	
	return(TRUE);
}

for(count=start;count<end;count++) {
	if(strcmp(tokens[count],"(") == 0) {		/* sub-expression */
		/* find end of sub-expression */

		endexpression=count+1;

		while(endexpression < end) {
			if(strcmp(tokens[endexpression],")") == 0) {
				commastart=count+1;

				/* check expression in list */

				for(commacount=count+1;commacount<endexpression-1;commacount++) {
			
					if(strcmp(tokens[commacount],",") == 0) {	/* found end */					
						if(IsValidExpression(tokens,commastart,commacount-1) == FALSE) return(FALSE);	

						commastart=commacount+1;	/* save start of next */
					}
				}
			

				return(IsValidExpression(tokens,count+1,endexpression-1)); /* sub-expression */
			}
		
			endexpression++;
		}

		count=endexpression+1;
	}

	if(IsOperator == FALSE) {
		/* Two condition characters can be together */
		if((strpbrk(tokens[count-1],ConditionCharacters) != NULL) && (strpbrk(tokens[count],ConditionCharacters) != NULL)) {
			IsValid=TRUE;
			IsOperator=!IsOperator;		/* make sure that the next token is treated as a non-operator */
		}
		else
		{
			if(IsValidVariableOrKeyword(tokens[count]) == FALSE) IsValid=FALSE;	/* not valid variable name */

			if(strpbrk(tokens[count],ValidExpressionCharacters) != NULL) {
				IsValid=FALSE;
			}
			else {
				IsValid=TRUE;	
			}
		}
	}
	else {
		if(strpbrk(tokens[count],ValidExpressionCharacters) == NULL) {
			IsValid=FALSE;
		}
		else {
			IsValid=TRUE;
		}
	}

	if((IsOperator == TRUE) && (count == (end-1)) && (strpbrk(tokens[end-1l],ValidExpressionCharacters) != NULL)) return(FALSE);

	IsOperator=!IsOperator;

	if(IsValid == FALSE) return(FALSE);
}
	
return(TRUE);
}

