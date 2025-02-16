#define EQUAL 1
#define NOTEQUAL 2
#define GTHAN 3
#define LTHAN 4
#define EQLTHAN 5
#define EQGTHAN 6

#define CONDITION_AND 0
#define CONDITION_OR 1
#define CONDITION_END 2

double EvaluateExpression(char *tokens[][MAX_SIZE],int start,int end);
void DeleteFromArray(char *arr[MAX_SIZE][MAX_SIZE],int start,int end,int deletestart,int deleteend);
int EvaluateSingleCondition(char *tokens[][MAX_SIZE],int start,int end);
int EvaluateCondition(char *tokens[][MAX_SIZE],int start,int end);

