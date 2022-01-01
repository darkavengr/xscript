#include <dlfcn.h>

int open_module(char *filename) {
int dlhandle;

dlhandle=dlopen(filename,RTLD_LAZY);			/* open library */
if(dlhandle == -1) return(-1);		/* can't open */

return(dlhandle);
}

