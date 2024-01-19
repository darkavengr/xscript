void itoa(int n, char s[]);
void reverse(char s[]);

/* itoa:  convert n to characters in s */
void itoa(int n, char s[]) {
int i;
int sign;

if ((sign = n) < 0) n = -n;          /* make n positive */
 i = 0;

 do {       /* generate digits in reverse order */
  s[i++] = n % 10 + '0';   /* get next digit */
 } while ((n /= 10) > 0);     /* delete it */

 if (sign < 0) {
  s[i++] = '-';
  s[i] = '\0';
 }

 reverse(s);
} 

void reverse(char s[]) {
int c;
int i;
int j;

for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
 c = s[i];
 s[i] = s[j];
 s[j] = c;
 }
}

