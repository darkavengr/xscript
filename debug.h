int set_breakpoint(int linenumber,char *functionname);
int clear_breakpoint(int linenumber,char *functionname);
int check_breakpoint(int linenumber,char *functionname);
void PrintVariable(vars_t *var);
void list_variables(char *name);

typedef struct {
 int linenumber;
 char *functionname[MAX_SIZE];
 struct BREAKPOINT *next;
} BREAKPOINT;
