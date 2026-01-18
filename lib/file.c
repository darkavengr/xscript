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
#include "variablesandfunctions.h"

//params[0]=filename (string)
//params[1]=mode (integer)

void xlib_open(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=open(params[0]->val->s,open(params[1]->val->i;

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

// params[0]=handle (integer)

void xlib_close(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=close(open(params[0]->val->i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=handle (integer)
//params[1]=buffer (string)
//params[2]=size (integer);

void xlib_read(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=read(params[0]->val->i,params[1]->val->s,params[2]->val->i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=handle (integer)
//params[1]=buffer (string)
//params[2]=size (integer);

void xlib_write(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=write(params[0]->val->i,params[1]->val->s,params[2]->val->i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=handle (integer)
//params[1]=position (integer)
//params[2]=whence (integer);

void xlib_seek(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=seek(params[0]->val->i,params[1]->val->i,params[2]->val->i);
if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=directory name (string)

void xlib_mkdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=mkdir(params[0]->val->s);
if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=directory name (string)

void xlib_rmdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=rmdir(params[0]->val->s);
if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=handle (integer)

void xlib_chdir(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=dup(params[0]->val->i);
if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=old handle (integer)
//params[1]=new handle (integer)

//params[0]=directory name (string)

void xlib_dup2(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=dup2(params[0]->val->i,params[0]->val->i,params[1]->val->i);
if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=filename (string)

//params[0]=directory name (string)

void xlib_unlink(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=unlink(params[0]->val->s);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

int find(bool IsFirst,char *filespec,FINDRESULT *outbuf) {
char *dirname[MAX_PATH];
struct stat statbuf;

if(IsFirst == TRUE) {
	if(GetDirectoryFromPath(filespec) == -1) {		/* path has no directory */
		findptr=opendir(".");
	}
	else
	{
		findptr=opendir(dirname);
	}
}

if(findptr == NULL) {				/* can't open directory */
	returnvalue.i=-1;
	returnvalue.systemerrorcode=errno;
	return;
}

if(readdir(dirptr) == NULL) {		/* find directory entry */
	returnvalue.i=-1;
	returnvalue.systemerrorcode=errno;
	return;
}

if(stat(dirptr->d_name,&statbuf) == -1) {	/* get file information */
	returnvalue.i=-1;
	returnvalue.systemerrorcode=errno;
	return;
}

/* copy entry information */

strncpy(outbuf->filename,dirptr->d_name,MAX_PATH);		/* filename */
outbuf->size=sb.st_size;				/* file size */
outbuf->uid=sb.st_uid;					/* user ID */
outbuf->gid=sb.st_gid;					/* group ID */
outbuf->permissions=sb.st_mode;				/* permissions */
outbuf->type=(sb.st_mode & S_IFMT);			/* type */

/* Create time */
outbuf->createtime_hours=sb.st_ctime.tm_hour;
outbuf->createtime_minutes=sb.st_ctime.tm_min;
outbuf->createtime_seconds=sb.st_ctime.tm_sec;
outbuf->createtime_day=sb.st_ctime.tm_mday;
outbuf->createtime_month=sb.st_ctime.tm_month;
outbuf->createtime_year=sb.st_ctime.tm_year;

/* access time */
outbuf->accesstime_hours=sb.st_ctime.tm_hour;
outbuf->accesstime_minutes=sb.st_ctime.tm_min;
outbuf->accesstime_seconds=sb.st_ctime.tm_sec;
outbuf->accesstime_day=sb.st_ctime.tm_mday;
outbuf->accesstime_month=sb.st_ctime.tm_month;
outbuf->accesstime_year=sb.st_ctime.tm_year;

/* modify time */
outbuf->modifytime_hours=sb.st_ctime.tm_hour;
outbuf->modifytime_minutes=sb.st_ctime.tm_min;
outbuf->modifytime_seconds=sb.st_ctime.tm_sec;
outbuf->modifytime_day=sb.st_ctime.tm_mday;
outbuf->modifytime_month=sb.st_ctime.tm_month;
outbuf->modifytime_year=sb.st_ctime.tm_year;

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

//params[0]=filename (string)
//params[1]=output (FINDRESULT *);


void xlib_findfirst(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=find(TRUE,params[0]->val->s,params[1]->udt);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;

}

void xlib_findnext(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval.i=find(FALSE,params[0]->val->s,params[1]->udt);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;
}

