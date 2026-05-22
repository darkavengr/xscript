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

for(count=start;count < end;count++) {
	if(strcmp(tokens[count],"(") == 0) {
		if(CheckFunctionExists(tokens[count - 1]) == 0) {
			while(count < end) {
				if(strcmp(tokens[count++] ,")") == 0) break;
			}

		}
		else if(IsVariable(tokens[count-1]) == TRUE) {
			for(countx=count + 1;countx < end;countx++) {
	 			if(strcmp(tokens[countx] ,")") == 0) break;
			}
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

for(count=0;count < exprcount;count++) {
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

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"*") == 0) { 
		sprintf(split_operands[count*2],"%.6g",atof(split_operands[count*2]) * atof(split_operands[(count*2)+1]));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;

	 } 
}
	
for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"+") == 0) {
		sprintf(split_operands[count*2],"%.6g",(atof(split_operands[count*2]) + atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count/2)+1,(count/2)+1);		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"-") == 0) { 
		sprintf(split_operands[count*2],"%.6g",(atof(split_operands[count*2]) - atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"<<") == 0) { 
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) << atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],">>") == 0) { 
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) >> atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

/* power */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"**") == 0) { 
		sprintf(split_operands[count*2],"%.6g",pow(atof(split_operands[count*2]),atof(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	 } 
}

/* modulus */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"%") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) % atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	}
}

/* bitwise not */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"!") == 0) {
		sprintf(split_operands[count*2],"%d",!atoi(split_operands[count*2]));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */

		count--;
		operatorcount--;
		operandcount--;
	}
}

/* bitwise and */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"&") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) & atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	}
}

/* bitwise or */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"|") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) | atoi(split_operands[(count*2)+1])));

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
	}
}

/* bitwise exclusive or */

for(count=0;count < operatorcount;count++)  {
	if(strcmp(split_operators[count],"^") == 0) {
		sprintf(split_operands[count*2],"%d",(atoi(split_operands[count*2]) ^ atoi(split_operands[(count*2)+1])));	

		DeleteFromArray(split_operators,0,operatorcount,count*2,count*2);		/* remove operator */
		DeleteFromArray(split_operands,0,operandcount,(count*2+1),(count*2+1));		/* remove second operand */

		count--;
		operatorcount--;
		operandcount--;
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
varval firstval;
varval secondval;
char *OperatorCharacters="=!<>";
int returnvalue;
char *exprtokens[MAX_SIZE][MAX_SIZE];
int substtc;

/* check kind of expression */

exprone=0;
exprtwo=0;

//printf("eval start=%d\n",start);
//printf("eval end=%d\n",end);
											
for(exprpos=start;exprpos < end;exprpos++) {
//	printf("eval tokens[%d]=%s\n",exprpos,tokens[exprpos]);

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
else
{
//	if((strpbrk(tokens[exprpos - 1],OperatorCharacters) != NULL) || (strpbrk(tokens[exprpos + 1],OperatorCharacters) != NULL)) {
//		SetLastError(INVALID_CONDITION);
//		return(-1);
// 	}
}

if(GetVariableType(tokens[start]) == VAR_STRING) {		/* comparing strings */
	substtc=SubstituteVariables(start,exprpos,tokens,exprtokens);

	if(ConatecateStrings(0,substtc,exprtokens,&firstval) == -1) {		/* join all the strings on the lines */
		if(firstval.s != NULL) free(firstval.s);
		return(-1);
	}

	substtc=SubstituteVariables(exprpos+1,end,tokens,exprtokens);

	if(ConatecateStrings(0,substtc,exprtokens,&secondval) == -1) {
		if(firstval.s != NULL) free(firstval.s);
		if(secondval.s != NULL) free(secondval.s);

		return(-1);
	}

	//printf("firstval.s=%s\n",firstval.s);
	//printf("secondval.s=%s\n",secondval.s);
	//asm("int $3");

	returnvalue=strncmp(firstval.s,secondval.s,MAX_SIZE);	/* reverse return value because strcmp returns 0 if strings match */

//	printf("returnvalue=%d\n",returnvalue);

	free(firstval.s);
	free(secondval.s);

	if(returnvalue == 0) return(TRUE);

	return(FALSE);
}

//if(IsValidExpression(tokens,start,exprpos - 1) == FALSE) {
//	SetLastError(INVALID_EXPRESSION);
//	return(-1);
//}

//if(IsValidExpression(tokens,exprpos + 1,end - 1) == FALSE) {
//	SetLastError(INVALID_EXPRESSION);
//	return(-1);
//}

substtc=SubstituteVariables(start,exprpos,tokens,exprtokens);
exprone=EvaluateExpression(exprtokens,0,substtc);				/* evaluate expressions */

substtc=SubstituteVariables(exprpos+1,end,tokens,exprtokens);
exprtwo=EvaluateExpression(exprtokens,0,substtc);

//printf("EvaluateSingleCondition() exprone=%.6g\n",exprone);
//printf("EvaluateSingleCondition() exprtwo=%.6g\n",exprtwo);

exprtrue=0;

switch(ifexpr) {

	case EQUAL:					/* exprone = exprtwo */
		//printf("equal=%d\n",exprone == exprtwo);
		//asm("int $3");

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

return(FALSE);
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

startcount=start;
count=0;

/* Do conditions in brackets first */

for(count=start;count < end;count++) {
	/* if sub-expression */

	if(strcmp(tokens[count],"(") == 0 && (CheckFunctionExists(tokens[count - 1]) == -1) && (IsVariable(tokens[count - 1]) == -1) ) {
	 	subcount++;
	 	startcount=count;

	 	while(count < exprend) {
	 		if(strcmp(tokens[count],")") == 0) {		/* end of sub-expression */
				evaltc=SubstituteVariables(startcount,count,tokens,evaltokens);
				if(evaltc == -1) return(-1);

	 			results[resultcount].result=EvaluateSingleCondition(evaltokens,0,count+1);

	 			resultcount++;

	 			if(strcmpi(temp[count],"AND") == 0) results[resultcount].and_or=CONDITION_AND;
	 			if(strcmpi(temp[count],"OR") == 0) results[resultcount].and_or=CONDITION_OR;

	 			count += subcount;		/* Add length of expression */
	    		}

			count++;
	  	}
	}

}

/* Do conditions outside brackets */

	startcount=0;

	for(count=0;count < outcount;count++) {
		if((strcmpi(temp[count],"AND") == 0) || (strcmpi(temp[count],"OR") == 0)) {// || (count >= outcount-1)) {
			evaltc=SubstituteVariables(startcount,count,tokens,evaltokens);
			if(evaltc == -1) return(-1);

			results[resultcount].result=EvaluateSingleCondition(evaltokens,0,evaltc);		

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

	//printf("resultcount=%d\n",resultcount);

	/* If there are no sub conditions, use whole expression */
	if(resultcount == 0) {
		//printf("SINGLE CONDITION\n");

		retval=EvaluateSingleCondition(tokens,start,end);
		return(retval);
	}

	overallresult=0;

	resultloopcount=0;

	while(resultloopcount < resultcount) {
		if(results[resultloopcount].and_or == CONDITION_AND) {		// and
			overallresult=results[resultloopcount].result && results[resultloopcount+1].result;

			resultloopcount += 2;
		}
		else if(results[resultloopcount].and_or == CONDITION_OR) {		// or
			overallresult=(results[resultloopcount].result || results[resultloopcount+1].result);
			resultloopcount += 2;
		}
		else if(results[resultloopcount].and_or == CONDITION_END) {		// end
			if(results[resultloopcount-1].and_or == CONDITION_AND) {
				overallresult = (overallresult && results[resultloopcount].result);
				resultloopcount += 2;
			}
			else if(results[resultloopcount-1].and_or == CONDITION_OR) {
				overallresult = (overallresult || results[resultloopcount].result);
				resultloopcount += 2;
			}
		}
		else
		{
			overallresult = (results[resultloopcount].result || results[resultloopcount+1].result);
		 	resultloopcount += 2;
		}
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
	if((char) *tokens == '"') return(TRUE);

	if(IsNumber(tokens[start]) == FALSE) return(FALSE);
	
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
		if(IsNumber(tokens[count]) == TRUE) {
			IsValid=TRUE;
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
