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

#include <dlfcn.h>

/*
 * Open Linux module
 *
 * In: char *filename	Filename to open
 * Returns -1 on error or module handle
 *
 */

int LoadModule(char *filename) {
int dlhandle;

dlhandle=dlopen(filename,RTLD_LAZY);			/* open library */
if(dlhandle == -1) return(-1);		/* can't open */

return(dlhandle);
}

