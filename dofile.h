int LoadFile(char *filename);
int ExecuteFile(char *filename);
int ExecuteLine(char *lbuf);
int function_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int print_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int import_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int if_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int for_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int return_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int get_return_value(varval *val);
int wend_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int next_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int while_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int end_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int else_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int elseif_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int endfunction_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int include_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int exit_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int declare_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int iterate_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int bad_keyword_as_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int type_statement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int quit_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int variables_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int continue_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int load_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int run_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int set_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int clear_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);

int TokenizeLine(char *linebuf,char *tokens[][MAX_SIZE],char *split);
int IsSeperator(char *token,char *sep);
int CheckSyntax(char *tokens[MAX_SIZE][MAX_SIZE],char *separators,int start,int end);
int touppercase(char *token);
char *ReadLineFromBuffer(char *buf,char *linebuf,int size);
int strcmpi(char *source,char *dest);
char *GetCurrentBufferAddress(void);
char *SetCurrentBufferAddress(char *addr);
void SetIsRunningFlag(void);
void ClearIsRunningFlag(void);
int GetIsRunningFlag(void);
void SetIsFileLoadedFlag(void);
int GetIsFileLoadedFlag(void);
int GetInteractiveModeFlag(void);
void SetInteractiveModeFlag(void);
void GetCurrentFile(char *buf);
void SetCurrentFile(char *buf);
void SetCurrentBufferPosition(char *pos);
char *GetCurrentBufferPosition(void);
void GetTokenCharacters(char *tbuf);


typedef struct {
 char *statement;
 char *endstatement;
 unsigned int (*call_statement)(int,void *);		/* function pointer */
 int is_block_statement;
} statement;

