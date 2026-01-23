#include "variablesandfunctions.h"

#ifndef DATATYPES_H
	#define DATATYPES_H

	typedef struct LIST {
		varval val;
		UserDefinedType *udt;
		struct LIST *prev;
		struct LIST *next;
	} LIST;
#endif

void SetDataValue(int type,varval *source,varval *dest);
void xlib_list_push_front(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_push_back(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_insert(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_delete(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_get(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_pop_front(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_pop_back(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_list_size(int paramcount,vars_t *params,libraryreturnvalue *returnval);

