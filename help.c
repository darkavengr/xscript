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
#include "size.h"
#include "errors.h"
#include "help.h"
/* display help */

int DisplayHelp(char *helpdir,char *topic) {
char *HelpFilename[MAX_SIZE];
char *b;
int linecount=0;
FILE *handle;
char *dirname[MAX_SIZE];
char *dptr;
char *aptr;

if(!*topic) {		                      /* get help file */
	sprintf(HelpFilename,"%s/help/index.txt",helpdir);
}
else
{
	sprintf(HelpFilename,"%s/help/%s.txt",helpdir,topic);
}

handle=fopen(HelpFilename,"r");
if(handle == NULL) {                 /* can't open file */
	SetLastError(HELP_TOPIC_DOES_NOT_EXIST);
	return(-1);
}

do {
	fgets(HelpFilename,MAX_SIZE,handle);

	printf("%s",HelpFilename);

	if(linecount++ == HELP_LINE_COUNT) {		/* end of paragraph */
		printf("-- Press any key to continue -- or type q to quit:");
		
		if(getc(stdin) == 'q') break;		/* quit help */
	
		linecount=0;
	}

} while(!feof(handle));               /* display text until end of file */

fclose(handle);

return(0);
}

