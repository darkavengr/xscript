

/* truncate line */

int strtrunc(char *str,int c) {
char *testbuf[100];
char *testpos;
int count;
int pos;

testpos=testbuf;

memset(testbuf,'\0',100);

//if(strlen(str) < c) return(0);
memcpy(testbuf,str,strlen(str)-c);
strcpy(str,testbuf);
}
