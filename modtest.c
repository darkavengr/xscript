#include <stdio.h>
#include <string.h>
#include "size.h"
#include "module.h"
#include "variablesandfunctions.h"

double double_test(double *a,double *b,double *c) {
puts("Double test module call\n");

printf("a=%.6g b=%.6g c=%.6g\n",*a,*b,*c);

return((*a)*(*b)*(*c));
}

char *string_test(char *str) {
printf("test string=%s\n",str);
return(str);
}

float single_test(float *a,float *b,float *c) {
puts("Single test module call\n");

printf("a=%.6f b=%.6f c=%.6f\n",*a,*b,*c);

return((*a)*(*b)*(*c));
}

int integer_test(int *a,int *b,int *c) {
puts("Integer test module call\n");

printf("a=%d b=%d c=%d\n",*a,*b,*c);

return((*a)*(*b)*(*c));
}

long int long_test(long int *a,long int *b,long int *c) {
puts("Long test module call\n");

printf("a=%ld b=%ld c=%ld\n",*a,*b,*c);

return((*a)*(*b)*(*c));
}

double mixed_test(double *a,float *b,int *c,long int *d,char *str) {
puts("Mixed variable type call\n");

printf("a=%.6g b=%.6f c=%d d=%ld str=%s\n",*a,*b,*c,*d,str);

return((*a)*(*b)*(*c)*(*d));
}

UserDefinedType *udt_test(UserDefinedType *testrecord) {
puts("User-defined type call\n");

printf("record name=%s\n",testrecord->name);

return(testrecord);
}


