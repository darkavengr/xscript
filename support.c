#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "support.h"
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"

/* support functions */

int GetDirectoryFromPath(char *path,char *dirbuf) {
char *aptr;
char *dptr;

if(strpbrk(path,"/") == NULL) return(-1);	/* no directory */

aptr=path;
dptr=dirbuf;

while(aptr != strrchr(path,'/')) {		/* from first character to last / in filename */
	*dptr++=*aptr++;
}

*dptr++=0;
return(0);
}

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

if(IsValidString(str) == FALSE) return;		/* not valid string */

/* copy filename without quotes */

strptr++;

while((char) *strptr != 0) {
	if((char) *strptr == '"') break;

	*bufptr++=*strptr++;	/* copy character */
}

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
 * Check if seperator
 *
 * In: token		Token to check
       sep		Seperator characters to check against
 *
 * Returns TRUE or FALSE
 *
 */
int IsSeperator(char *token,char *sep) {
char *SepPtr;

if(*token == 0) return(TRUE);
	
SepPtr=sep;

while(*SepPtr != 0) {
	if((char) *SepPtr++ == (char) *token) return(TRUE);
}

if(IsStatement(token) == TRUE) return(TRUE);

return(FALSE);
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
		if((char) *tokenptr != '.') return(FALSE);
	}

	tokenptr++;
}

return(TRUE);
}
