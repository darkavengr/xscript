int InitializeFunctions(void);
void FreeFunctions(void);
int CreateVariable(char *name,int type,int xsize,int ysize);
int UpdateVariable(char *name,varval *val,int x,int y);
int ResizeArray(char *name,int x,int y);
int GetVariableValue(char *name,varval *val);
int GetVariableType(char *name);
int ParseVariableName(char *name,varsplit *split);
int RemoveVariable(char *name);
int DeclareFunction(char *tokens[MAX_SIZE][MAX_SIZE],int funcargcount);
int CheckFunctionExists(char *name);
int atoi_base(char *hex,int base);
int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE]);
double CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end);

