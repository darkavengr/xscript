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

#include "define.h"

/*
 * Module functions
 *
 */

MODULES *modules=NULL;

/*
 * Load module
 *
 * In: char *modulename	Filename of module to load
 * Returns error number on error or result on success
 *
 * Returns -1 on error or 0 on success
 */

int AddModule(char *modulename) {
MODULES *next;
MODULES *last;
int handle;
char *filename[MAX_SIZE];
char *modpath;
char *moddirs[MAX_SIZE][MAX_SIZE];
int tc=0;
int count;

modpath=getenv("XSCRIPT_MODULE_PATH");				/* get module path */

if(modpath == NULL)  {
 PrintError(NO_MODULE_PATH);	/* no module path warning */

 strcpy(modpath,".");		/* use currnet directory */
}

tc=TokenizeLine(modpath,moddirs,":");			/* tokenize line */

for(count=0;count<tc;count++) {			/* loop through path array */

/* get module filename without extension
   LoadModule adds the extension for portability reasons */

 sprintf(modulename,"%s\\%s",moddirs[count],filename);	

 handle=LoadModule(filename);
 if(handle == -1) continue;			/* can't open module */

 if(modules == NULL) {			/* first in list */
  modules=malloc(sizeof(MODULES));
  if(modules == NULL) {
   PrintError(NO_MEM);
   return(-1);
  }

  last=modules;
 }
 else
 {
  next=modules;

  while(next != NULL) {
   last=next;
   next=next->next;
  }
 }

 last->next=malloc(sizeof(MODULES));
 if(modules == NULL) {
  PrintError(NO_MEM);
  return(-1);
 }

 next=last->next;
 strcpy(next->modulename,filename);
 next->dlhandle=handle;
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

intGetModuleHandle(char *module) {
MODULES *next;
next=modules;

/* search through linked list */

while(next != NULL) {
 if(strcmpi(next->modulename,module) == 0) return(next->dlhandle);		/* found module */
 
 next=next->next;
}

return(-1);
}

