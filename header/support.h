#include "size.h"

int IsValidString(char *str);
void StripQuotesFromString(char *str,char *buf);
void RemoveNewline(char *line);
int strcmpi(char *source,char *dest);
void ToUpperCase(char *token);
int IsNumber(char *token);
int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split);

