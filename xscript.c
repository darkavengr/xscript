#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "define.h"
#include "defs.h"

int main(int argc, char **argv) {
int count;
char *args[MAX_SIZE];
char *filename=NULL;
char *buffer[MAX_SIZE];
varval cmdargs;

init_funcs();

/* intialized command-line arguments */

addvar("argv",VAR_STRING,argc,0);			/* add commandline variable */

memset(cmdargs.s,0,MAX_SIZE);

for(count=0;count<argc;count++) {
 strcpy(cmdargs.s,argv[count]);
 updatevar("argv",&cmdargs.s,count,0);
}


if(argc == 1) {					/* no args */ 
 while(1) {
  fgets(buffer,MAX_SIZE,stdin);			/* read line */
  doline(buffer);
 }

}
else
{
 dofile(argv[1]);						/* do file */
}

free_funcs();

exit(0);
}
