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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"

/*
 * Main function
 *
 * In: int argc		Number of arguments
 *     char **argv	Arguments
 *
 * Returns: Nothing
 *
 */

int main(int argc, char **argv) {
int count;
char *args[MAX_SIZE];
char *filename=NULL;
char *buffer[MAX_SIZE];
varval cmdargs;

InitializeFunctions();						/* Initialize functions */

/* intialize command-line arguments */

CreateVariable("argc",VAR_INTEGER,argc,0);
cmdargs.i=argc;
UpdateVariable("argc",&cmdargs,count,0);

CreateVariable("argv",VAR_STRING,argc,0);			/* add command line arguments variable */

memset(cmdargs.s,0,MAX_SIZE);

for(count=0;count<argc;count++) {
 strcpy(cmdargs.s,argv[count]);
 UpdateVariable("argv",&cmdargs,count,0);
}

if(argc == 1) {					/* no arguments */ 
 while(1) {
  fgets(buffer,MAX_SIZE,stdin);			/* read line */
  ExecuteLine(buffer);				/* execute line */
 }

}
else
{
 ExecuteFile(argv[1]);						/* execute file */
}

FreeFunctions();

exit(0);
}
