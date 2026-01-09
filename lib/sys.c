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

/* IMPORTANT: XScript passes ALL parameters as POINTERS (see module.c) */

int xlib_exec(char *filename,char *args) {
int returnvalue=fork();		/* fork process */

if(returnvalue == 0) {		/* child process */
	return(execlp(filename,args));	/* execute process */
}
else if(returnvalue == 0) {		/* child process */
	return(-1);
}

return(0);
}

int xlib_exit(int *returncode) {
return(exit(*returncode));
}

int xlib_sleep(int *sleeptime) {
return(sleep(*sleeptime));
}

int xlib_dup(int *handle) {
return(dup(*handle));
}

int xlib_dup2(int *handle,int *newhandle) {
return(dup2(*handle,*newhandle));
}

int xlib_chdir(char *dirname) {
return(chdir(dirname));
}

int xlib_getpid(void) {
return(getpid());
}

int xlib_getenv(char *name,char *outbuf) {
char *envptr=getenv(name);		/* get enviroment variable */

if(envptr == NULL) return(-1);	/* not found */

strcpy(outbuf,envptr);
return(0);
}

int xlib_wait(int *status) {
return(wait(*stataus));
}

int xlib_kill(int *pid,int *signal) {
return(kill(*pid,*signal));
}
 
