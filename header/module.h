#define MODULE_BINARY	1
#define MODULE_SCRIPT	2

int AddModule(char *modulename);

typedef struct {
 char *modulename[MAX_SIZE];
 void *dlhandle;
 int flags;
 char *StartInBuffer;
 char *EndInBuffer;
 struct MODULES *last;
 struct MODULES *next;
} MODULES;

int AddModule(char *filename);
void *GetModuleHandle(char *module);
MODULES *GetModuleEntry(char *module);
void FreeModulesList(void);
int AddToModulesList(MODULES *entry);
MODULES *GetCurrentModuleInformationFromBufferAddress(char *address);
void *LoadModule(char *filename);
void *GetLibraryFunctionAddress(void *handle,char *name);
void *GetModuleAddress(int handle,char *name);
void GetModuleFileExtension(char *buf);

