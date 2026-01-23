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

/* FILE I/O stub functions */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include "variablesandfunctions.h"
#include "support.h"
#include "file.h"

int GetDirectoryFromPath(char *path,char *dirbuf) {
char *aptr;
char *dptr;

if(strpbrk(path,"/") == NULL) return(-1);	/* no directory */

aptr=path;
dptr=dirbuf;

while(aptr != strrchr(path,'/')) {		/* from first character to last / in filename */
	*dptr++=*aptr++;
}

*dptr++=0;
return(0);
}

//params[0]=filename (string)
//params[1]=mode (integer)

void xlib_open(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=open(params[0].val->s,params[1].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

// params[0]=handle (integer)

void xlib_close(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=close(params[0].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

//params[0]=handle (integer)
//params[1]=buffer (string)
//params[2]=size (integer);

void xlib_read(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=read(params[0].val->i,params[1].val->s,params[2].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

//params[0]=handle (integer)
//params[1]=buffer (string)
//params[2]=size (integer);

void xlib_write(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=write(params[0].val->i,params[1].val->s,params[2].val->i);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

//params[0]=handle (integer)
//params[1]=position (integer)
//params[2]=whence (integer);

void xlib_seek(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=lseek(params[0].val->i,params[1].val->i,params[2].val->i);
if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

//params[0]=directory name (string)

void xlib_mkdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=mkdir(params[0].val->s,params[1].val->i);
if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

//params[0]=directory name (string)

void xlib_rmdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=rmdir(params[0].val->s);
if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

//params[0]=filename (string)

//params[0]=directory name (string)

void xlib_unlink(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=unlink(params[0].val->s);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

int find(DIR *findptr,char *filespec,UserDefinedType *outbuf) {
char *dirname[MAX_PATH];
struct stat statbuf;
struct dirent *dirptr;
struct tm *temptime;
UserDefinedTypeField *usertypefield;

if(findptr == NULL) {
	if(GetDirectoryFromPath(filespec,dirname) == -1) {		/* path has no directory */
		findptr=opendir(".");
	}
}

if(findptr == NULL) return(-1);				/* can't open directory */

if(readdir(dirptr) == NULL) return(-1);		/* find directory entry */

if(stat(dirptr->d_name,&statbuf) == -1) return(-1);	/* get file information */

/* copy entry information */

strncpy(GetUDTFieldPointer(outbuf,"name",0,0)->fieldval->s,dirptr->d_name,MAX_PATH);		/* filename */
GetUDTFieldPointer(outbuf,"size",0,0)->fieldval->i=statbuf.st_size;				/* file size */
GetUDTFieldPointer(outbuf,"uid",0,0)->fieldval->i=statbuf.st_uid;					/* user ID */
GetUDTFieldPointer(outbuf,"gid",0,0)->fieldval->i=statbuf.st_gid;					/* group ID */
GetUDTFieldPointer(outbuf,"permissions",0,0)->fieldval->i=statbuf.st_mode;				/* permissions */
GetUDTFieldPointer(outbuf,"type",0,0)->fieldval->i=(statbuf.st_mode & S_IFMT);			/* type */

temptime=gmtime(statbuf.st_ctime);

/* Create time */
GetUDTFieldPointer(outbuf,"createtime_hour",0,0)->fieldval->i=temptime->tm_hour;
GetUDTFieldPointer(outbuf,"createtime_minutes",0,0)->fieldval->i=temptime->tm_min;
GetUDTFieldPointer(outbuf,"createtime_seconds",0,0)->fieldval->i=temptime->tm_sec;
GetUDTFieldPointer(outbuf,"createdate_day",0,0)->fieldval->i=temptime->tm_mday;
GetUDTFieldPointer(outbuf,"createdate_month",0,0)->fieldval->i=temptime->tm_mon;
GetUDTFieldPointer(outbuf,"createdate_year",0,0)->fieldval->i=temptime->tm_year;

temptime=gmtime(statbuf.st_atime);

/* access time */
GetUDTFieldPointer(outbuf,"accesstime_hour",0,0)->fieldval->i=temptime->tm_hour;
GetUDTFieldPointer(outbuf,"accesstime_minutes",0,0)->fieldval->i=temptime->tm_min;
GetUDTFieldPointer(outbuf,"accesstime_seconds",0,0)->fieldval->i=temptime->tm_sec;
GetUDTFieldPointer(outbuf,"accessdate_day",0,0)->fieldval->i=temptime->tm_mday;
GetUDTFieldPointer(outbuf,"accessdate_month",0,0)->fieldval->i=temptime->tm_mon;
GetUDTFieldPointer(outbuf,"accessdate_year",0,0)->fieldval->i=temptime->tm_year;

temptime=gmtime(statbuf.st_mtime);

/* modify time */
GetUDTFieldPointer(outbuf,"modifytime_hour",0,0)->fieldval->i=temptime->tm_hour;
GetUDTFieldPointer(outbuf,"modifytime_minutes",0,0)->fieldval->i=temptime->tm_min;
GetUDTFieldPointer(outbuf,"modifytime_seconds",0,0)->fieldval->i=temptime->tm_sec;
GetUDTFieldPointer(outbuf,"modifydate_day",0,0)->fieldval->i=temptime->tm_mday;
GetUDTFieldPointer(outbuf,"modifydate_month",0,0)->fieldval->i=temptime->tm_mon;
GetUDTFieldPointer(outbuf,"modifydate_year",0,0)->fieldval->i=temptime->tm_year;
return(0);
}

//params[0]=filename (string)
//params[1]=output (FINDRESULT *);

void xlib_findfirst(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=find(NULL,params[1].val->s,params[2].udt);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

//params[0]=find handle (DIR *)
//params[0]=filename (string)
//params[1]=output (FINDRESULT *);

void xlib_findnext(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=find(params[0].val->a,params[1].val->s,params[2].udt);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

