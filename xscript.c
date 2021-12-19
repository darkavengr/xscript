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

memset(args,0,MAX_SIZE);

for(count=1;count<argc;count++) {
 strcat(args,argv[count]);			/* get command line */

 if(count != argc-1) strcat(args," ");			/* get command line */

}

init_funcs();

addvar("command",0,0);			/* add commandline variable */
//updatevar("command",&cmdargs.s,0,0);

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

//sfree_funcs();

exit(0);
}
