#include <windows.h>

int open_module(char *filename) {
int dlhandle;

dlhandle=LoadLibrary(filename);			/* open library */
if(dlhandle == NULL) return(-1);		/* can't open */

return(dlhandle);
}

