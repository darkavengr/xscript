int AddModule(char *modulename);
int GetModuleHandle(char *module);
int LoadModule(char *filename);

typedef struct {
 char *modulename[MAX_SIZE];
 void  (*dladdr)(void);			/* function pointer */
 void *dlhandle;
 struct MODULES *last;
 struct MODULES *next;
} MODULES;
