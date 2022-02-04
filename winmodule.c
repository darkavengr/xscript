#include <windows.h>

int open_module(char *filename) {
int dlhandle;

strcat(filename,".dll");			/* add extension */

dlhandle=LoadLibrary(filename);			/* open library */
if(dlhandle == NULL) return(-1);		/* can't open */

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

 dlcall=GetProcAddress(handle,function);		/* get address of function */
 if(dlcall == NULL) return(FUNCTION_NOT_FOUND);			/* error */

 dlcall(function);						/* call function */
 return(0);
}
 
