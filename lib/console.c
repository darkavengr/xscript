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
#include "console.h"
#include "variablesandfunctions.h"

/* IMPORTANT: XScript passed ALL parameters as POINTERS (see module.c) */

int xlib_readline(int paramcount,vars_t *params,libraryreturnvalue *result) {
result=NULL;

gets(params[0]->val->s;
return(0);
}

