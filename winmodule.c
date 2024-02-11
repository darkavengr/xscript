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

#include <windows.h>

int LoadModule(char *filename);

/*
 * Load windows module
 *
 * In: char *filename			Filename of file to load
 *
 * Returns -1 on error or module handle on success
 *
 */

int LoadModule(char *filename) {
int dlhandle;

dlhandle=LoadLibrary(filename);			/* open library */
if(dlhandle == NULL) {
	SetLastError(MISSING_LIBSYM);
	return(-1);		/* can't open */
}

return(dlhandle);
}

