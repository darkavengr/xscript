/*  XScript runtime library Version 0.0.1
   (C) Matthew Boote 2026

   This file is part of XScript.

   XScript is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   XScript is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with XScript.  If not, see <https://www.gnu.org/Licenses/>.
*/

/* DataType functions */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "variablesandfunctions.h"
#include "datatypes_list.h"
#include "support.h"

void SetDataValue(int type,varval *source,varval *dest) {
if(type == VAR_NUMBER) {		/* double */
	dest->d=(double) source->d;
}
else if(type == VAR_SINGLE) {		/* single */
	dest->f=source->f;
}
else if(type == VAR_INTEGER) {		/* integer */
	dest->i=source->i;
}
else if(type == VAR_LONG) {		/* long */
	dest->l=source->l;
}
else if(type == VAR_BOOLEAN) {		/* boolean */
	dest->b=source->b;
}
else if(type == VAR_ANY) {		/* any */
	dest->a=source->a;
}

return;
}

//params[0]=list (list)
//params[1]=new entry

void xlib_list_push_front(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *liststart=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a; /* get pointer to start of list */
LIST *newstart;
int DataType=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i;	/* get list data type */

newstart=calloc(1,sizeof(LIST));	/* allocate entry at end */
if(newstart == NULL) {		/* can't allocate memory */
	returnval->val.i=-1;
	returnval->systemerrornumber=errno;
	return;
}

if(liststart == NULL) {		/* empty list */
	liststart=newstart;
}
else
{
	newstart->next=liststart;		/* insert at start of list */
	liststart->prev=newstart;		/* point to previous entry */
}

GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a=newstart->next;		/* update pointer to start of list */

/* copy data */

if(DataType == VAR_UDT) {		/* UDT */
	memcpy(liststart->udt,params[1].udt,sizeof(UserDefinedType));
}
else
{
	SetDataValue(params[0].type_int,params[0].val,params[1].val);
}

GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a=liststart; /* set end of list */
GetUDTFieldPointer(params[0].val->a,"NumberOfEntries",0,0)->fieldval->i++;	/* increment number of entries */
return;
}

//params[0]=list (list)

void xlib_list_pop_front(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *liststart;
LIST *entry;

int DataType=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i;	/* get list data type */

liststart=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a; /* get pointer to start of list */

if(DataType == VAR_UDT) {		/* UDT */
	memcpy(returnval->udt,liststart->udt,sizeof(UserDefinedType));
}
else
{
	SetDataValue(DataType,&liststart->val,&returnval->val);
}

/* remove entry from start of list */
entry=liststart;		/* get pointer to first entry */
liststart=liststart->next;	/* skip over first entry */
liststart->prev=NULL;		/* no previous entry */
free(entry);			/* release it */
return;
}

//params[0]=list (list)
//params[1]=new entry

void xlib_list_push_back(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *listend=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListEnd",0,0)->fieldval->a; /* get pointer to end of list */
int DataType=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i;	/* get list data type */
LIST *newentry;

if(listend == NULL) {		/* empty list */
	listend=calloc(1,sizeof(LIST));	/* allocate entry */
	if(listend == NULL) {		/* can't allocate memory */
		returnval->val.i=-1;
		returnval->systemerrornumber=errno;
		return;
	}
}
else
{
	newentry=calloc(1,sizeof(LIST));	/* allocate entry at end */
	if(newentry == NULL) {		/* can't allocate memory */
		returnval->val.i=-1;
		returnval->systemerrornumber=errno;
		return;
	}	

	listend->next=newentry;		/* insert new entry at end */
	newentry->prev=listend;		/* set pointer to previous */

	listend=listend->next;
	
}

/* copy data */

if(DataType == VAR_UDT) {		/* UDT */
	memcpy(listend->udt,params[1].udt,sizeof(UserDefinedType));
}
else
{
	SetDataValue(params[0].type_int,params[0].val,params[1].val);
}

GetUDTFieldPointer(params[0].val->a,"ListEnd",0,0)->fieldval->a=listend; /* set end of list */
GetUDTFieldPointer(params[0].val->a,"NumberOfEntries",0,0)->fieldval->i++;	/* increment number of entries */
return;
}

//params[0]=list (list)

void xlib_list_pop_back(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *listend;
LIST *entry;
int DataType=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i;	/* get list data type */

listend=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListEnd",0,0)->fieldval->a; /* get pointer to end of list */

if(DataType == VAR_UDT) {		/* UDT */
	memcpy(returnval->udt,listend->udt,sizeof(UserDefinedType));
}
else
{
	SetDataValue(DataType,&listend->val,&returnval->val);
}

/* remove entry from end of list */

free(listend->prev->next);	/* free entry */
listend->prev->next=NULL;	/* remove last entry */

return;
}

//params[0]=list (LIST *)
//params[1]=entry number
//params[2]=new entry

void xlib_list_insert(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *next;
LIST *entry;
int NumberOfEntries=GetUDTFieldPointer(params[0].val->a,"NumberOfEntries",0,0)->fieldval->i; /* get number of entries */
int count=0;
int DataType=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i;	/* get list data type */

if(params[0].val->i == 0) {		/* inserting at start */
	xlib_list_prepend(paramcount,params,returnval);
	return;
}
else if(params[0].val->i == NumberOfEntries) {		/* inserting at end */
	xlib_list_append(paramcount,params,returnval);
	return;
}

next=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a; /* get pointer to start of list */

if(next == NULL) {		/* empty list */
		entry=calloc(1,sizeof(LIST));	/* allocate new start */
		if(entry == NULL) {		/* can't allocate memory */
			returnval->val.i=-1;
			returnval->systemerrornumber=errno;
			return;
		}
}
else
{
	while(next != NULL) {

		if(count == params[1].val->i) {		/* found entry */
			entry=calloc(1,sizeof(LIST));	/* allocate new start */
			if(entry == NULL) {		/* can't allocate memory */
				returnval->val.i=-1;
				returnval->systemerrornumber=errno;
				return;
			}

			
			next->prev=entry;		/* link previous entry to new entry */
			entry->next=next;		/* link new entry to next entry */

			if(DataType == VAR_UDT) {		/* UDT */
				memcpy(next->udt,params[1].udt,sizeof(UserDefinedType));
			}
			else
			{
				SetDataValue(params[0].type_int,params[0].val,params[1].val);
			}

			return;
		}

		count++;
		next=next->next;
	}
}

return;
}

//params[0]=list (LIST *)
//params[1]=entry number

void xlib_list_delete(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *next;
LIST *entry;
int NumberOfEntries=GetUDTFieldPointer(params[0].val->a,"NumberOfEntries",0,0)->fieldval->i; /* get number of entries */
int count=0;

next=(LIST *) GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a; /* get pointer to start of list */

if(next == NULL) {		/* empty list */
	returnval->val.i=-1;
	returnval->systemerrornumber=ENODATA;
	return;
}

if(params[0].val->i == 0) {		/* deleting at start */
	entry=next->next;		/* skip over first entry */
	free(next);			/* release it */
	return;
}

while(next != NULL) {

	if(count == params[1].val->i) {		/* found entry */
		next->prev=next->next;	/* link previous entry to next-next entry */
		free(next);
		return;
	}

	count++;
	next=next->next;
}

returnval->val.i=-1;
returnval->systemerrornumber=ENODATA;
return;
}

void xlib_list_get(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
LIST *next;
LIST *previous;
LIST *entry;
int datatype=GetUDTFieldPointer(params[0].val->a,"DataType",0,0)->fieldval->i; /* get number of entries */
int count=0;

next=GetUDTFieldPointer(params[0].val->a,"ListStart",0,0)->fieldval->a; /* point to start of list */

if(next == NULL) {		/* empty list */
	returnval->val.i=-1;
	returnval->systemerrornumber=errno;
	return;
}

while(next != NULL) {

	if(count == params[1].val->i) {		/* found entry */
		if(datatype == VAR_UDT) {		/* UDT */
			memcpy(next->udt,params[1].udt,sizeof(UserDefinedType));
		}
		else
		{
			SetDataValue(params[0].type_int,params[0].val,params[1].val);
		}

		return;
	}

		count++;
		next=next->next;
}

return;
}

void xlib_list_size(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=GetUDTFieldPointer(params[0].val->a,"NumberOfEntries",0,0)->fieldval->i; /* get number of entries */
return;
}

