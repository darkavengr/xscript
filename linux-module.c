#include <dlfcn.h>

/*
 * Open Linux module
 *
 * In: char *filename	Filename to open
 * Returns -1 on error or module handle
 *
 */

int open_module(char *filename) {
int dlhandle;

dlhandle=dlopen(filename,RTLD_LAZY);			/* open library */
if(dlhandle == -1) return(-1);		/* can't open */

return(dlhandle);
}

