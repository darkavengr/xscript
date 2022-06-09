#include <windows.h>

/*
 * Load windows
 *
 * In: char *filename			Filename of file to load
 *
 * Returns -1 on error or module handle on success
 *
 */

int open_module(char *filename) {
int dlhandle;

dlhandle=LoadLibrary(filename);			/* open library */
if(dlhandle == NULL) return(-1);		/* can't open */

return(dlhandle);
}

