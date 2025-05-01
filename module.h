#define MODULE_BINARY	1
#define MODULE_SCRIPT	2

int AddModule(char *modulename);

typedef struct {
 char *modulename[MAX_SIZE];
 void  (*dladdr)(void);			/* function pointer */
 void *dlhandle;
 int flags;
 struct MODULES *last;
 struct MODULES *next;
} MODULES;
