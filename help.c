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
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "size.h"
#include "errors.h"
#include "help.h"

int DisplayHelp(char *topic) {
char *HelpFilename="xscript.help";

if(!*topic) return(DisplayHelpTopic(HelpFilename,"index"));                      /* display index */

return(DisplayHelpTopic(HelpFilename,topic));
}

int DisplayHelpTopic(char *helpfile,char *topic) {
FILE *handle;
char *LineBuffer[MAX_SIZE];
bool FoundTopic=FALSE;
char *LineTokens[MAX_SIZE][MAX_SIZE];

handle=fopen(helpfile,"r");		/* open help file */
if(!handle) {				/* can't open file */
	SetLastError(FILE_NOT_FOUND);
	return(-1);
}

while(!feof(handle)) {
	fgets(LineBuffer,MAX_SIZE,handle);			/* get data */

	RemoveNewline(LineBuffer);				/* remove newline character */

	TokenizeLine(LineBuffer,LineTokens," ");			/* tokenize line */

	if((strcmpi(LineTokens[0],"%TOPIC") == 0) &&  (strcmpi(LineTokens[1],topic) == 0)) FoundTopic=TRUE; /* found topic */

	if(FoundTopic == TRUE) {		/* display topic */
		if(strcmpi(LineTokens[0],"%ENDTOPIC") == 0) {	/* end of topic */
			FoundTopic=FALSE;	
			
			fclose(handle);
			
			SetLastError(NO_ERROR);
			return(0);
		}

		if(strcmpi(LineTokens[0],"%TOPIC") != 0) printf("%s\n",LineBuffer);
	}
}

fclose(handle);

SetLastError(HELP_TOPIC_DOES_NOT_EXIST);
return(-1);
}

