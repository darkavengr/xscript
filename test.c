#include <stdio.h>
#include <string.h>

int deletefromarray(char *arr[255][255],int start,int end,int n) {
 int count;
 char *temp[255][255];
 int oc=0;

 for(count=start+n;count<end;count++) {
   strcpy(temp[oc++],arr[count]);    
 }

 for(count=0;count<oc;count++) {
   strcpy(arr[count],temp[count]);  
 }

 arr[count][0]=NULL;
return;
}

int main()
{
char *array[255][255];
int count;

strcpy(array[0],"apple");
strcpy(array[1],"pear");
strcpy(array[2],"peach");
strcpy(array[3],"plum");

deletefromarray(array,0,4,1);

for(count=0;count < 4;count++) {
 printf("%s\n",array[count]);
}

}

