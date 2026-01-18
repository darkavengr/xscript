#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"

void testfunc(int paramcount,vars_t *params,libraryreturnvalue *result) {
puts("Test module call\n");

printf("double a=%.6g\n",params[0].val->d);
printf("integer b=%d\n",params[1].val->i);
printf("single c=%f\n",params[2].val->f);
printf("long=%ld\n",params[3].val->l);

result->val.d=params[0].val->d*2;
return;
}


void testfunc_string(int paramcount,vars_t *params,libraryreturnvalue *result) {
printf("%s\n",params[0].val->s);

result->val.s=calloc(1,MAX_SIZE);
if(result->val.s == NULL) {
	result->returnvalue=-1;
	result->systemerrornumber=errno;
}

strncpy(result->val.s,"This has been returned",MAX_SIZE);
return;
}

