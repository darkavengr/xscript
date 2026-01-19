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

/* System functions */

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "variablesandfunctions.h"
#include "sys.h"

//params[0]=filename (string)
//params[1]=arguments (string)

void xlib_exec(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
int returnvalue=fork();		/* fork process */

if(returnvalue == 0) {		/* child process */
	returnval->val.i=execlp(params[0].val->s,params[1].val->s);	/* execute process */

	if(returnval->val.i == -1) {
		returnval->systemerrornumber=errno;
		return;
	}
}
else if(returnvalue == -1) {		/* child process */
		returnval->systemerrornumber=errno;
		return;
}

return(0);
}

// params[0]=exit value

void xlib_exit(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
exit(params[0].val->i);
if(returnval->val.i == -1) returnval->systemerrornumber=errno;

return;
}

// params[0]=number of seconds to sleep

void xlib_sleep(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=sleep(params[0].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

// params[0]=handle

void xlib_dup(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=dup(params[0].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

// params[0]=old handle
// params[1]=new handle

void xlib_dup2(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=dup2(params[0].val->i,params[1].val->i);
}

// params[0]=directory name

void xlib_chdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=chdir(params[0].val->s);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

void xlib_getpid(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=getpid();

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

// params[0]=enviroment variable name

void xlib_getenv(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.s=getenv(params[0].val->s);

if(returnval->val.s == NULL) returnval->systemerrornumber=errno;
return;
}

// params[0]=process to wait on

void xlib_wait(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=wait(params[0].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}


// params[0]=process to send signal to

void xlib_kill(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=kill(params[0].val->i,params[1].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

