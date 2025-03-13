int set_breakpoint(int linenumber,char *functionname);
int clear_breakpoint(int linenumber,char *functionname);
int check_breakpoint(int linenumber,char *functionname);
void PrintVariableValue(vars_t *var);
int list_variables(char *name);

typedef struct {
 int linenumber;
 char *functionname[MAX_SIZE];
 struct BREAKPOINT *next;
} BREAKPOINT;
