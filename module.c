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
#include "errors.h"
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"
#include "dofile.h"

int AddModule(char *modulename);
int GetModuleHandle(char *module);

/*
 * Module functions
 *
 */

MODULES *modules=NULL;
MODULES *modules_end=NULL;

/*
 * Load module
 *
 * In: char *modulename	Filename of module to load
 * Returns error number on error or result on success
 *
 * Returns -1 on error or 0 on success
 */

int AddModule(char *modulename) {
int handle;
char *filename[MAX_SIZE];
char *modpath;
char *moddirs[MAX_SIZE][MAX_SIZE];
int tc=0;
int count;
MODULES ModuleEntry;

modpath=getenv("XSCRIPT_MODULE_PATH");				/* get module path */

if(modpath == NULL)  {
 SetLastError(NO_MODULE_PATH);	/* no module path warning */

 strcpy(modpath,".");		/* use current directory */
}

tc=TokenizeLine(modpath,moddirs,":");			/* tokenize line */

for(count=0;count<tc;count++) {			/* loop through path array */

/* get module filename without extension
   LoadModule adds the extension for portability reasons */

	sprintf(modulename,"%s\\%s",moddirs[count],filename);	

	handle=LoadModule(filename);
	if(handle == -1) continue;			/* can't open module */

	/* add module entry */
	strcpy(ModuleEntry.modulename,filename);
	ModuleEntry.dlhandle=handle;
	ModuleEntry.flags=MODULE_BINARY;

	AddToModulesList(&ModuleEntry);
}

return(0);
}

/*
 * Get module handle
 *
 * In: char *module	Module name
 *	
 * Returns -1 on error or module handle
 *
 */

int GetModuleHandle(char *module) {
MODULES *next=modules;

/* search through linked list */

while(next != NULL) {
	if(strcmpi(next->modulename,module) == 0) return(next->dlhandle);		/* found module */
 
	next=next->next;
}

return(-1);
}

/*
 * Free modules list
 *
 * In: Nothing
 *	
 * Returns: Nothing
 *
 */

void FreeModulesList(void) {
MODULES *next=modules;
MODULES *nextptr;

/* search through linked list */

while(next != NULL) {
	nextptr=next->next;
	free(next);
	next=nextptr;
}

modules=NULL;
return;
}

int AddToModulesList(MODULES *entry) {
if(modules == NULL) {			/* first in list */
	modules=malloc(sizeof(MODULES));
	if(modules == NULL) {
		SetLastError(NO_MEM);
	  	return(-1);
	}

	modules_end=modules;
	modules_end->last=NULL;
}
else
{
	modules_end->next=malloc(sizeof(MODULES));
	
	if(modules_end->next == NULL) {
 		SetLastError(NO_MEM);
 		return(-1);
	}

	modules_end=modules_end->next;
}

memcpy(modules_end,entry,sizeof(MODULES));

SetLastError(0);
return(0);
}

/*
 * Get module entry from start address
 *
 * In: buf	Module information buffer
 *	
 * Returns -1 on error or module handle
 *
 */

int GetCurrentModuleInformationFromBufferAddress(MODULES *buf) {
MODULES *next=modules;
char *buffer=GetCurrentFileBufferPosition();

/* search through linked list */

while(next != NULL) {
//	printf("%lX %lX %lX\n",buffer,next->StartInBuffer,next->EndInBuffer);

	if((buffer >= next->StartInBuffer) && (buffer <= next->EndInBuffer)) {		/* found entry */
		memcpy(buf,next,sizeof(MODULES));
		return(0);
	}
 
	next=next->next;
}

return(-1);
}


