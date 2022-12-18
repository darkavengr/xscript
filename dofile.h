char *ReadLineFromBuffer(char *buf,char *linebuf,int size);

int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int for_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int include_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int break_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int run_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int continue_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int type_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
double doexpr(char *tokens[][MAX_SIZE],int start,int end);
int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split);

extern int SubstituteVariables(int start,int end,char *tokens[][MAX_SIZE]);

