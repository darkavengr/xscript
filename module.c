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

int AddModule(char *filename) {
MODULES ModuleEntry;

ModuleEntry.dlhandle=LoadModule(filename);	/* load module */
if(ModuleEntry.dlhandle == -1) return(-1);

/* add module entry */
strcpy(ModuleEntry.modulename,filename);
ModuleEntry.flags=MODULE_BINARY;
AddToModulesList(&ModuleEntry);		/* add to modules list */

SetLastError(NO_ERROR);
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

while(next != NULL) {
	nextptr=next->next;

	if(next->StartInBuffer != NULL) free(next->StartInBuffer);		/* free script module buffer */

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
modules_end->next=NULL;

SetLastError(0);
return(0);
}

/*
 * Get module entry from start address
 *
 * In: Nothing
 *	
 * Returns: Pointer to module information or NULL on error
 *
 */

MODULES *GetCurrentModuleInformationFromBufferAddress(char *address) {
MODULES *next=modules;
char *buffer=GetCurrentFileBufferPosition();

/* search through module list for buffer address */

while(next != NULL) {
	if((address >= next->StartInBuffer) && (address <= next->EndInBuffer)) return(next);	/* found entry */
 
	next=next->next;
}

return(NULL);
}


