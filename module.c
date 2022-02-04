#include "define.h"

MODULES *modules=NULL;

int add_module(char *modulename) {
MODULES *next;
MODULES *last;
int handle;
char *filename[MAX_SIZE];
char *modpath;
char *moddirs[MAX_SIZE][MAX_SIZE];
int tc=0;
int count;

modpath=getenv("XSCRIPT_MODULE_PATH");				/* get module path */

if(modpath == NULL) {				/* no module path */
 print_error(NO_MODULE_PATH);
 return(-1);
}

tc=tokenize_line(modpath,moddirs,":");			/* tokenize line */

for(count=0;count<tc;count++) {			/* loop through path array */

/* get module filename without extension
   open_module adds the extension for portability reasons */

 sprintf(modulename,"%s\\%s",moddirs[count],filename);	

 handle=open_module(filename);
 if(handle == -1) continue;			/* can't open module */

 if(modules == NULL) {			/* first in list */
  modules=malloc(sizeof(MODULES));
  if(modules == NULL) {
   print_error(NO_MEM);
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
  print_error(NO_MEM);
  return(-1);
 }

 next=last->next;
 strcpy(next->modulename,filename);
 next->dlhandle=handle;
}

return;
}

int module_get_handle(char *module) {
MODULES *next;
next=modules;

while(next != NULL) {
 if(strcmpi(next->modulename,module) == 0) return(next->dlhandle);		/* found module */
 
 next=next->next;
}

return(-1);
}

