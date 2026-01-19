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

int find(DIR *findptr,char *filespec,FINDRESULT *outbuf) {
char *dirname[MAX_PATH];
struct stat statbuf;
struct dirent *dirptr;
struct tm *temptime;

if(findptr == NULL) {
	if(GetDirectoryFromPath(filespec,dirname) == -1) {		/* path has no directory */
		findptr=opendir(".");
	}
	else
	{
		findptr=opendir(dirname);
	}
}

if(findptr == NULL) return(-1);				/* can't open directory */

if(readdir(dirptr) == NULL) return(-1);		/* find directory entry */

if(stat(dirptr->d_name,&statbuf) == -1) return(-1);	/* get file information */

/* copy entry information */

strncpy(outbuf->filename,dirptr->d_name,MAX_PATH);		/* filename */
outbuf->size=statbuf.st_size;				/* file size */
outbuf->uid=statbuf.st_uid;					/* user ID */
outbuf->gid=statbuf.st_gid;					/* group ID */
outbuf->permissions=statbuf.st_mode;				/* permissions */
outbuf->type=(statbuf.st_mode & S_IFMT);			/* type */

temptime=gmtime(statbuf.st_ctime);

/* Create time */
outbuf->createtime_hours=temptime->tm_hour;
outbuf->createtime_minutes=temptime->tm_min;
outbuf->createtime_seconds=temptime->tm_sec;
outbuf->createdate_day=temptime->tm_mday;
outbuf->createdate_month=temptime->tm_mon;
outbuf->createdate_year=temptime->tm_year;

temptime=gmtime(statbuf.st_atime);

/* access time */
outbuf->accesstime_hours=temptime->tm_hour;
outbuf->accesstime_minutes=temptime->tm_min;
outbuf->accesstime_seconds=temptime->tm_sec;
outbuf->accessdate_day=temptime->tm_mday;
outbuf->accessdate_month=temptime->tm_mon;
outbuf->accessdate_year=temptime->tm_year;

temptime=gmtime(statbuf.st_mtime);

/* modify time */
outbuf->modifytime_hours=temptime->tm_hour;
outbuf->modifytime_minutes=temptime->tm_min;
outbuf->modifytime_seconds=temptime->tm_sec;
outbuf->modifydate_day=temptime->tm_mday;
outbuf->modifydate_month=temptime->tm_mon;
outbuf->modifydate_year=temptime->tm_year;

return(0);
}

//params[0]=0
//params[1]=filename (string)
//params[2]=output (FINDRESULT *);


void xlib_findfirst(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=find(params[1].val->i,params[1].val->s,params[2].udt);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;

}

void xlib_findnext(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=find(params[1].val->i,params[0].val->s,params[1].udt);

if(returnval->val.i == -1) returnval->systemerrornumber=errno;
return;
}

