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

/* Network functions */

#include <unistd.h>
#ifdef __linux__
	#include <netdb.h>
	#include <sys/socket.h>
	#include <sys/types.h> 
	#include <netinet/in.h>
 	#include <arpa/inet.h>
	#include <sys/stat.h>
#endif

#ifdef _WIN32
	#include <winsock2.h>
#endif

#include "variablesandfunctions.h"

// params[0]=domain (integer)
// params[0]=type (integer)
// params[0]=protocol (integer)

void xlib_socket(int paramcount,vars_t *params,libraryreturnvalue *returnvalue) {
returnvalue.i=socket(params[0]->val.i,params[1]->val.i,params[2]->val.i));

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;

return;
}

// params[0]=socket (integer)
// params[1]=hostname (string)
// params[2]=port (integer);
// params[3]=type (integer);

void xlib_connect(int paramcount,vars_t *params,libraryreturnvalue *returnvalue) {
struct sockaddr_in service;

service.sin_family=params[2]->val.i;
service.sin_addr.s_addr=inet_addr(params[1]->val.s);
service.sin_port=htons(params[2]->val.i);

returnvalue.i=connect(params[0]->val.i,&service,sizeof(service)));

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;
}

// params[0]=socket (integer)
// params[0]=buffer (string):
// params[0]=size (integer)

void xlib_recv(int paramcount,vars_t *params,libraryreturnvalue *returnvalue) {
returnvalue.i=recv(params[0]->val.i,params[1]->val.s,params[2]->val.i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;
}

// params[0]=socket (integer)
// params[0]=buffer (string):
// params[0]=size (integer)

void xlib_send(int paramcount,vars_t *params,libraryreturnvalue *returnvalue) {
returnvalue.i=send(params[0]->val.i,params[1]->val.s,params[2]->val.i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;
}

// params[0]=socket (integer)
// params[1]=socket information (Socket)
// params[2]=flags (integer)

void xlib_accept(int paramcount,vars_t *params,libraryreturnvalue *returnvalue) {
returnvalue.i=accept(params[0]->val.i,params[1]->val.udt,params[2]->val.i);

if(returnvalue.i == -1) returnvalue.systemerrorcode=errno;
return;
}

