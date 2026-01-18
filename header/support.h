int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split);
int IsSeperator(char *token,char *sep);
void ToUpperCase(char *token);
void StripQuotesFromString(char *str,char *buf);
int strcmpi(char *source,char *dest);
int GetDirectoryFromPath(char *path,char *dirbuf);
int IsNumber(char *token);

