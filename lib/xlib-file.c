/*  XScript runtime library Version 0.0.1
   (C) Matthew Boote 2020

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

/* IMPORTANT: XScript passed ALL parameters as POINTERS (see module.c) */

int xlib_open(char *filename,int *mode) {
return(open(filename,*mode);
}

int xlib_close(int *handle) {
return(close(*handle));
}

int xlib_read(int *handle,void *buffer,int *size) {
return(read(*handle,buffer,*size);
}

int xlib_read(int *handle,void *buffer,int *size) {
return(read(*handle,buffer,*size);
}

int xlib_write(int *handle,void *buffer,int *size) {
return(write(*handle,buffer,*size);
}

int xlib_seek(int *handle,int *position,int *whence) {
return(seek(*handle,*position,*whence);
}

int xlib_tell(int *handle) {
return(tell(*handle);
}

int xlib_chdir(char *dirname) {
return(chdir(dirname));
}

int xlib_mkdir(char *dirname) {
return(mkdir(dirname));
}

int xlib_rmdir(char *dirname) {
return(rmdir(dirname));
}

int xlib_dup(int *handle) {
return(dup(*handle));
}

int xlib_dup2(int *oldhandle,int *newhandle) {
return(dup2(*oldhandle,*newhandle));
}

int xlib_unlink(char *filename) {
return(unlink(filename));
}

