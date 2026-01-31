/*  XScript runtime library Version 0.0.1
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
   along with XScript.  If not, see <https://www.gnu.org/Licenses/>.
*/

/* Console I/O stub functions */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "variablesandfunctions.h"
#include "module.h"
#include "console.h"
#include "size.h"
#include "support.h"

void xlib_inkey(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.s=calloc(1,MAX_SIZE);	/* allocate return string */
if(returnval->val.s == NULL) {
	returnval->systemerrornumber=errno;
	returnval->returnvalue=-1;
	return;
}

fgets(returnval->val.s,1,stdin);
}

void xlib_input(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.s=calloc(1,MAX_SIZE);	/* allocate return string */
if(returnval->val.s == NULL) {
	returnval->systemerrornumber=errno;
	returnval->returnvalue=-1;
	return;
}

fgets(returnval->val.s,MAX_SIZE,stdin);	/* read line */
RemoveNewline(returnval->val.s);		/* remove newline */
}

