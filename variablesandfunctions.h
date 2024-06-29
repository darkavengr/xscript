#include <stdbool.h>

#define FOR_STATEMENT 1
#define IF_STATEMENT 2
#define WHILE_STATEMENT 4
#define FUNCTION_STATEMENT 8

#define VAR_NUMBER  0
#define VAR_STRING  1
#define VAR_INTEGER 2
#define VAR_SINGLE  3
#define VAR_UDT     4

#define ARRAY_SUBSCRIPT 1
#define ARRAY_SLICE	2

#define SUBST_VAR	1
#define SUBST_FUNCTION	2

#define MAX_INCLUDE 10

#define MAX_NEST_COUNT 256

typedef struct {
 double d;
 char *s;
 int i;
 float f;
 int type;
} varval;

typedef struct {
 char *fieldname[MAX_SIZE];
 varval *fieldval;
 int xsize;
 int ysize;
 int type;
 struct UserDefinedTypeField *next;
} UserDefinedTypeField;

typedef struct  {
 char *name[MAX_SIZE];
 UserDefinedTypeField *field;
 struct UserDefinedType *next;
} UserDefinedType;

typedef struct {
 char *varname[MAX_SIZE];
 varval *val;
 UserDefinedType *udt;
 int xsize;
 int ysize;
 char *udt_type[MAX_SIZE];
 int type_int;
 struct vars_t *next;
} vars_t;

typedef struct {
 char *name[MAX_SIZE];
 int x;
 int y;
 int arraytype;
 char *fieldname[MAX_SIZE];
 int fieldx;
 int fieldy;
} varsplit;

typedef struct {
 char *bufptr;
 int linenumber;
 struct SAVEINFORMATION *last;
 struct SAVEINFORMATION *next;
} SAVEINFORMATION;

typedef struct {
 char *name[MAX_SIZE];
 char *fieldname[MAX_SIZE];
 char *funcstart;
 int funcargcount;
 char *returntype[MAX_SIZE];
 int type_int;
 vars_t *parameters;
 int linenumber;
 struct functions *next;
} functions;

typedef struct {
 char *name[MAX_SIZE];
 char *callptr;
 int linenumber;
 SAVEINFORMATION *saveinformation;
 SAVEINFORMATION *saveinformation_top;
 vars_t *vars;
 int stat;	
 char *returntype[MAX_SIZE];
 int type_int;
 int lastlooptype;
 struct FUNCTIONCALLSTACK *next;
 struct FUNCTIONCALLSTACK *last;
} FUNCTIONCALLSTACK;

typedef struct {
 varval val;
 UserDefinedType *udt;
 bool has_returned_value;
} functionreturnvalue;

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
double CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end);
int ReturnFromFunction(void);
int atoi_base(char *hex,int base);
int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE],char *out[][MAX_SIZE]);
int ConatecateStrings(int start,int end,char *tokens[][MAX_SIZE],varval *val);
int CheckVariableType(char *typename);
int IsVariable(char *varname);
int PushFunctionCallInformation(FUNCTIONCALLSTACK *func);
int PopFunctionCallInformation(void);
int FindFirstVariable(vars_t *var);
int FindNextVariable(vars_t *var);
int FindVariable(char *name,vars_t *var);
UserDefinedType *GetUDT(char *name);
int AddUserDefinedType(UserDefinedType *newudt);
int CopyUDT(UserDefinedType *source,UserDefinedType *dest);
vars_t *GetVariablePointer(char *name);
int GetFieldValueFromUserDefinedType(char *varname,char *fieldname,varval *val,int fieldx,int fieldy);
int IsValidVariableType(char *type);
int GetFieldTypeFromUserDefinedType(char *varname,char *fieldname);
void GetCurrentFunctionName(char *buf);
int GetCurrentFunctionLine(void);
void SetCurrentFunctionLine(int linenumber);
void SetFunctionCallPtr(char *ptr);
void SetFunctionFlags(int flags);
void ClearFunctionFlags(int flags);
int GetFunctionFlags(void);
char *GetSaveInformationBufferPointer(void);
int GetSaveInformationLineCount(void);
int GetFunctionReturnType(void);
int PushSaveInformation(void);
int PopSaveInformation(void);
int GetVariableXSize(char *name);
int GetVariableYSize(char *name);
int IsFunction(char *name);
SAVEINFORMATION *GetSaveInformation(void);
void InitializeMainFunction(char *args);
void DeclareBuiltInVariables(char *args);
void GetCurrentFunctionFilename(char *buf);

