#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "support.h"
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"

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

/*
*  Get pointer to UDT field
* 
*  In:  udt		User defined type
	fieldname	Field name
	fieldx		Field X subscript	
	fieldy		Field Y subscript	
* 
*  Returns: Pointer to field on success or NULL on error
*
*/
UserDefinedTypeField *GetUDTFieldPointer(UserDefinedType *udt,char *fieldname,int fieldx,int fieldy) {
udtptr=udt->field;

while(udtptr != NULL) {
	if(strcmp(udtptr->fieldname,fieldname) == 0) return(udtptr);	/* found field */				
 
	udtptr=udtptr->next;
}

return(NULL);
}

