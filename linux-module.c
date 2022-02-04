#include <dlfcn.h>
#include <stdarg.h>

#include "define.h"
#define NULL 0

/*
 * open module
 */
int open_module(char *filename) {
int dlhandle;
va_list args;

strcat(filename,".o");			/* add extension */

dlhandle=dlopen(filename,RTLD_LAZY);			/* open library */
if(dlhandle == -1) return(-1);		/* can't open */

return(dlhandle);
}

/*
 * call function from module
 */

int module_call_function(char *module,char *function,...) {
 int handle;
 void * (*dlcall)(function);			/* function pointer for call */

 
 handle=module_get_handle(module);			/* get handle */
 if(handle == -1) return(MODULE_NOT_FOUND);

 dlcall=dlsym(handle,function);				/* get address of function */
 if(dlcall == NULL) return(FUNCTION_NOT_FOUND);			/* error */

 dlcall(function);						/* call function */
 return(0);
}
 


