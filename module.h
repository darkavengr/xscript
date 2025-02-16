int AddModule(char *modulename);

typedef struct {
 char *modulename[MAX_SIZE];
 void  (*dladdr)(void);			/* function pointer */
 void *dlhandle;
 struct MODULES *last;
 struct MODULES *next;
} MODULES;
