int InitializeFunctions(void);
void FreeFunctions(void);
int CreateVariable(char *name,char *type,int xsize,int ysize);
int UpdateVariable(char *name,char *fieldname,varval *val,int x,int y);
int ResizeArray(char *name,int x,int y);
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy);
int GetVariableType(char *name);
int ParseVariableName(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end,varsplit *split);
int RemoveVariable(char *name);
int DeclareFunction(char *tokens[MAX_SIZE][MAX_SIZE],int funcargcount);
int CheckFunctionExists(char *name);
int atoi_base(char *hex,int base);
int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE],char *out[][MAX_SIZE]);
double CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end);
int IsVariable(char *varname);
UserDefinedType *GetUDT(char *name);

