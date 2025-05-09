#include <stdio.h>
#include <stdarg.h>
#include "errors.h"
#include "size.h"
#include "module.h"

/* dodgy code */

size_t save_tc;
vars_t *varnext=NULL;

int CallModule(int tc,char *tokens[MAX_SIZE][MAX_SIZE]) {
int count;

/* build call parameters */

callptr=GetFunctionAddress(tokens[0]);		/* get pointer to function */

if(save_tc >= 7) {
	tempbuf=malloc((tc-7)*sizeof(size_t));	/* allocate temporary buffer */

	for(count=tc-7;count > 0;count--) {
		tempbuf[count]=atoi(tokens[count]);
	}

	asm volatile("mov %%rsp,%0":: "a"(oldrsp));		/* save stack pointer */
	asm volatile("mov %%rbp,%0":: "a"(oldrbp));		/* save stack base */

	asm volatile("mov %0,%%rbp":: "a"(&tokens[save_tc-1]));	/* load stack base */
	asm volatile("mov %0,%%rsp":: "a"(&tokens[save_tc-7]);	/* load stack pointer */
}

	asm volatile("mov %0,%%rdi":: "a"(tokens[1]));	/* fist six parameters are in registers */
	asm volatile("mov %0,%%rsi":: "a"(tokens[2]));
	asm volatile("mov %0,%%rdx":: "a"(tokens[3]));
	asm volatile("mov %0,%%rcx":: "a"(tokens[4]));
	asm volatile("mov %0,%%r8":: "a"(tokens[5]));
	asm volatile("mov %0,%%r9":: "a"(tokens[6]));

	callptr();			/* call function */

	asm volatile("mov %%eax,%0":: "a"(returnvalue));	/* get return value */
	
	asm volatile("mov %0,%%rsp":: "a"(oldrsp));		/* restore stack pointer */
	asm volatile("mov %0,%%rbp":: "a"(oldrbp));		/* restore stack base */

	return(returnvalue);
}
else
{
	asm volatile("mov %0,%%rdi":: "a"(tokens[1]));	/* fist six parameters are in registers */
	asm volatile("mov %0,%%rsi":: "a"(tokens[2]));
	asm volatile("mov %0,%%rdx":: "a"(tokens[3]));
	asm volatile("mov %0,%%rcx":: "a"(tokens[4]));
	asm volatile("mov %0,%%r8":: "a"(tokens[5]));
	asm volatile("mov %0,%%r9":: "a"(tokens[6]));

	callptr();			/* call function */

	asm volatile("mov %%eax,%0":: "a"(returnvalue));	/* get return value */

	return(returnvalue);
}


