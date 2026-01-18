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
#include "sys.h"
#include "variablesandfunctions.h"

//params[0]=filename (string)
//params[1]=arguments (string)

void xlib_exec(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
int returnvalue=fork();		/* fork process */

if(returnvalue == 0) {		/* child process */
	returnval.i=execlp(params[0]->val->s,params[1]->val->s);	/* execute process */

	if(returnval.i == -1) {
		returnval.systemerrorcode=errno;
		return;
	}
}
else if(returnvalue == -1) {		/* child process */
		returnval.systemerrorcode=errno;
		return;
	}
}

return(0);
}

// params[0]=exit value

void xlib_exit(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=exit(params[0]->val->i);
if(returnval.i == -1) returnval.systemerrorcode=errno;

return;
}

// params[0]=number of seconds to sleep

void xlib_sleep(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=sleep(params[0]->val->i);

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}

// params[0]=handle

void xlib_dup(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=dup(params[0]->val->i);

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}

// params[0]=old handle
// params[1]=new handle

void xlib_dup2(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=dup2(params[0]->val->i,params[1]->val->i);
}

// params[0]=directory name

void xlib_chdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=chdir(params[0]->val->s);

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}

void xlib_getpid(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=getpid();

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}

// params[0]=enviroment variable name

void xlib_getenv(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.s=getenv(params[0]->val->s);

if(returnval.s == NULL) returnval.systemerrorcode=errno;
return;
}

// params[0]=process to wait on

void xlib_wait(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=getpid(params[0]->val->i);

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}


// params[0]=process to send signal to

void xlib_kill(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=kill(params[0]->val->i);

if(returnval.i == -1) returnval.systemerrorcode=errno;
return;
}

