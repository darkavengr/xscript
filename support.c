#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "size.h"
#include "support.h"

/* support functions */

int IsValidString(char *str) {
char *s;

if(*str != '"') return(FALSE);

s=(str+strlen(str))-1;		/* point to end */

if(*s != '"') return(FALSE);

return(TRUE);
}

void StripQuotesFromString(char *str,char *buf) {
char *strptr=str;
char *bufptr=buf;

if(IsValidString(str) == TRUE) strptr++;	/* skip quote character */

/* copy filename without quotes */

while((char) *strptr != 0) {
	if((char) *strptr == '"') break;

	*bufptr++=*strptr++;	/* copy character */
}

*bufptr++=0;

return;
}

void RemoveNewline(char *line) {
char *b;

if(strlen(line) > 1) {
	b=(line+strlen(line))-1;
	if((*b == '\n') || (*b == '\r')) *b=0;
}

return;
}


/*
 * Compare string case insensitively
 *
 * In: source		First string
       dest		Second string
 *
 * Returns: 0 if matches, positive or negative number otherwise
 *
 */
int strcmpi(char *source,char *dest) {
char *sourcetemp[MAX_SIZE];
char *desttemp[MAX_SIZE];

/* create copies of the string and convert them to uppercase */

memset(sourcetemp,0,MAX_SIZE);
memset(desttemp,0,MAX_SIZE);

strncpy(sourcetemp,source,MAX_SIZE);
strncpy(desttemp,dest,MAX_SIZE);

ToUpperCase(sourcetemp);
ToUpperCase(desttemp);

return(strncmp(sourcetemp,desttemp,MAX_SIZE));		/* return result of string comparison */
}
	 
/*
 * Convert to uppercase
 *
 * In: char *token	String to convert
 *
 * Returns: nothing
 *
 */

void ToUpperCase(char *token) {
char *tokenptr;
	
tokenptr=token;

while(*tokenptr != 0) { 	/* until end */
	if(((char) *tokenptr >= 'a') &&  ((char) *tokenptr <= 'z')) *tokenptr -= 32;	/* convert to lower case if upper case */
  	tokenptr++;
}

return;
}

/*
*  Determine if string is numeric or not
* 
*  In: string
* 
*  Returns: TRUE or FALSE
*
*/
int IsNumber(char *token) {
char *tokenptr=token;

while((char) *tokenptr != 0) {
	if( ((char ) *tokenptr < '0') || ((char ) *tokenptr > '9')) {
		if(((char) *tokenptr == 'e') || ((char) *tokenptr == '-') || ((char) *tokenptr == '.')) {
			return(TRUE);
		}
		else
		{
			return(FALSE);
		}
	}

	tokenptr++;
}

return(TRUE);
}

/*
 * Tokenize string
 *
 * In: linebuf				Line to tokenize
       tokens[MAX_SIZE][MAX_SIZE]	Token array output
 *
 * Returns -1 on error or token count on success
 *
 */

/* TODO: Rewrite this function */

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split) {
char *token;
int tc;
int count;
char *d;
char *s;
int IsSeperatorCharacter;
char *lastcharptr;
char *nextcharptr;

token=linebuf;

while(((char) *token == ' ') || ((char) *token == '\t')) token++;	/* skip leading whitespace characters */

/* tokenize line */

	tc=0;
	
	d=tokens[0];
	memset(d,0,MAX_SIZE);				/* clear line */
	
	while((char) *token != 0) {
		IsSeperatorCharacter=FALSE;

		if(*token == '"' ) {		/* quoted text */ 
	   		*d++=*token++;

	   		while((char)  *token != 0) {
	    			*d++=*token++;

	    			if(*(d-1) == '"') break;		/* quoted text */	

	  		}

	  		tc++;
	 	}
	 	else
	 	{
	  		s=split;

	  		while(*s != 0) {
	    			if((char) *token == *s) {		/* seperator found */

			    		if((strlen(tokens[tc]) != 0) && (*token != '.')) tc++;
	    
			    		IsSeperatorCharacter=TRUE;
			    		d=tokens[tc]; 			

			    		memset(d,0,MAX_SIZE);				/* clear line */

					/* ignore spaces between tokens; this then allows the following be be equivalent:
						2 + 2
						2+2
					*/

			    		if((char) *token != ' ') {
						/* handle seperators that are multi-character operators */

						if((char) *token == '*') {		/* ** */
							nextcharptr=token;
							nextcharptr++;
		
							if((char) *nextcharptr != '*') {
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '>') {		/* >> or >= */
							nextcharptr=token;
							nextcharptr++;
										
							if((char) *nextcharptr == '=') {
						      		*d++=*token++;
						      		*d++=*token++;
							}
							else if((char) *nextcharptr == '>') {
						      		*d++=*token++;
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];
						}
						else if((char) *token == '<') {		/* << or <= */					
							nextcharptr=token;
							nextcharptr++;
	
							if((char) *nextcharptr == '=') {
						      		*d++=*token++;
						      		*d++=*token++;
							}
							else if((char) *nextcharptr == '<') {
						      		*d++=*token++;
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '!') {	
							nextcharptr=token;
							nextcharptr++;
										
							if((char) *nextcharptr != '=') {
								tc++;
						      		*d++=*token++;
							}
							else
							{
						      		*d++=*token++;
						      		*d++=*token++;
							}

							IsSeperatorCharacter=FALSE;
							d=tokens[++tc];	
						}
						else if((char) *token == '.') {		/* can be either decimal point or member */
							nextcharptr=token;
							nextcharptr++;

							if((char) *nextcharptr == 0) return(tc);

							lastcharptr=token;
							lastcharptr--;

							if( (((char)  *lastcharptr >= '0') && ((char)  *lastcharptr <= '9')) && 
							    (((char) *nextcharptr >= '0') && ((char) *nextcharptr <= '9'))) {
								*d++=*lastcharptr;
								*d++=*token++;
								*d++=*token++;
							}
							else
							{
								tc++;
								token++;
							}
						}
						else if((char) *token == '-') {		/* can be either part of the
											token as a negative sign or a subtract operator token */
							lastcharptr=token;
							lastcharptr--;

							nextcharptr=token;
							nextcharptr++;

							if( ((char) *lastcharptr == ' ') && ((char) *nextcharptr != ' ')) {
								*d++=*token++;
								*d++=*token++;
								tc++;
							}
							else
							{
								*d++=*token++;

								d=tokens[++tc];

								*d++=*token++;
								tc += 2;
							}		

						}
						else		/* single character token */
						{
			      				*d++=*token++;	
				     		     	d=tokens[++tc];
						}
			    		}
			    		else
					{
						token++;		/* ignore spaces next to tokens */

						if((char) *token == 0) break;
					}
	   	}

	 	s++;
	 }

	 if(IsSeperatorCharacter == FALSE) *d++=*token++; /* non-token character */
	}
}

if(strlen(tokens[tc]) > 0) tc++;		/* if there is data in the last token, increment the counter so it is accounted for */

return(tc);
}

