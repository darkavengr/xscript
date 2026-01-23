#include <stdio.h>
#include <unistd.h>

#define MAX_PATH 255

#ifndef FILE_H
	#define FILE_H

	typedef struct {
		char *filename[MAX_PATH];
		long size;
		int uid;
		int gid;
		int permissions;
		int type;
		int createtime_hours;
		int createtime_minutes;
		int createtime_seconds;
		int createdate_day;
		int createdate_month;
		int createdate_year;
		int accesstime_hours;
		int accesstime_minutes;
		int accesstime_seconds;
		int accessdate_day;
		int accessdate_month;
		int accessdate_year;
		int modifytime_hours;
		int modifytime_minutes;
		int modifytime_seconds;
		int modifydate_day;
		int modifydate_month;
		int modifydate_year;
	} FINDRESULT;
#endif

void xlib_open(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_close(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_read(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_write(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_seek(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_mkdir(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_rmdir(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_chdir(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_dup2(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_unlink(int paramcount,vars_t *params,libraryreturnvalue *returnval);
int find(DIR *findptr,char *filespec,UserDefinedType *outbuf);
void xlib_findfirst(int paramcount,vars_t *params,libraryreturnvalue *returnval);
void xlib_findnext(int paramcount,vars_t *params,libraryreturnvalue *returnval);
int GetDirectoryFromPath(char *path,char *dirbuf);


