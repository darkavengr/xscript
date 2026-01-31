#include "variablesandfunctions.h"

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split);
int IsSeperator(char *token,char *sep);
void ToUpperCase(char *token);
void StripQuotesFromString(char *str,char *buf);
int strcmpi(char *source,char *dest);
int IsNumber(char *token);
UserDefinedTypeField *GetUDTFieldPointer(UserDefinedType *udt,char *fieldname,int fieldx,int fieldy);
void RemoveNewline(char *line);

