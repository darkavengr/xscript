#include <stdbool.h>
#include "module.h"

#define FOR_STATEMENT			1
#define IF_STATEMENT			2
#define WHILE_STATEMENT			4
#define FUNCTION_STATEMENT		8
#define FUNCTION_INTERACTIVE_MODE	16
#define WAS_ITERATED			32

#define VAR_NUMBER  0
#define VAR_STRING  1
#define VAR_INTEGER 2
#define VAR_SINGLE  3
#define VAR_LONG    4
#define VAR_BOOLEAN 5
#define VAR_UDT     6

#define ARRAY_SUBSCRIPT 1
#define ARRAY_SLICE	2

#define SUBST_VAR	1
#define SUBST_FUNCTION	2

#define MAX_INCLUDE 10

#define MAX_NEST_COUNT 256
#define DEFAULT_TYPE_INT VAR_NUMBER

#ifndef VARIABLESANDFUNCTIONS_H
	#define VARIABLESANDFUNCTIONS_H

	typedef struct {
	double d;
	char *s;
	int i;
	float f;
	long int l;
	bool b;
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

	typedef struct vartype {
		char *varname[MAX_SIZE];
		varval *val;
		UserDefinedType *udt;
		int xsize;
		int ysize;
		char *udt_type[MAX_SIZE];
		int type_int;
		bool IsConstant;
		struct vartype *next;
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
		struct SAVEINFORMATION *next;
	} SAVEINFORMATION;

	typedef struct func {
		char *name[MAX_SIZE];
		char *funcstart;
		int funcargcount;
		char *returntype[MAX_SIZE];
		int type_int;
		vars_t *parameters;
 		vars_t *parameters_end;
		int linenumber;
		bool WasDeclaredInInteractiveMode;
		struct func *last;
		struct func *next;
	} functions;

	typedef struct FUNCTIONCALLSTACK {
		char *name[MAX_SIZE];
		char *callptr;
		int startlinenumber;
		int currentlinenumber;
		SAVEINFORMATION *saveinformation_top;
		vars_t *vars;
		vars_t *vars_end;
		vars_t *initialparameters;
		int stat;	
		char *returntype[MAX_SIZE];
		int type_int;
		int lastlooptype;
		MODULES *moduleptr;
		struct FUNCTIONCALLSTACK *next;
	} FUNCTIONCALLSTACK;

	typedef struct {
		varval val;
		UserDefinedType *udt;
		bool has_returned_value;
	} functionreturnvalue;
#endif

void FreeFunctions(void);
int CreateVariable(char *name,char *type,int xsize,int ysize);
int UpdateVariable(char *name,char *fieldname,varval *val,int x,int y,int fieldx,int fieldy);
int ResizeArray(char *name,int x,int y);
int GetVariableValue(char *name,char *fieldname,int x,int y,varval *val,int fieldx,int fieldy);
int GetVariableType(char *name);
int ParseVariableName(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end,varsplit *split);
int RemoveVariable(char *name);
int DeclareFunction(char *tokens[MAX_SIZE][MAX_SIZE],int funcargcount);
int CheckFunctionExists(char *name);
int CallFunction(char *tokens[MAX_SIZE][MAX_SIZE],int start,int end);
void ReturnFromFunction(void);
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
char *GetCurrentFunctionName(void);
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
int InitializeMainFunction(char *progname,char *args);
void DeclareBuiltInVariables(char *progname,char *args);
void FreeFunctionsAndVariables(void);
void FreeVariablesList(vars_t *vars);
int IsNumber(char *str);
functions *GetFunctionPointer(char *name);
FUNCTIONCALLSTACK *GetFunctionCallStackTop(void);
void FreeVariableValues(varval *val,int type,int xsize,int ysize);

