int CallIfStatement(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int IsStatement(char *statement);
int IsEndStatementForStatement(char *statement,char *endstatement);
int IsBlockStatement(char *statement);

#define FOR_STATEMENT			1
#define IF_STATEMENT			2
#define WHILE_STATEMENT			4
#define FUNCTION_STATEMENT		8
#define FUNCTION_INTERACTIVE_MODE	16
#define WAS_ITERATED			32
#define REPEAT_STATEMENT		64

