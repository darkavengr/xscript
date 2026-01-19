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

/* Mathematical stub functions */

#include <math.h>
#include <time.h>
#include "variablesandfunctions.h"
#include "math.h"

void xlib_abs(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.i=abs(params[0].val->i);
return;
}

void xlib_arctan(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=atan(params[0].val->d);
return;
}

void xlib_cos(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=cos(params[0].val->d);
return;
}

void xlib_sin(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=sin(params[0].val->d);
return;
}

void xlib_tan(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=tan(params[0].val->d);
return;
}

void xlib_log(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=log(params[0].val->d);
return;
}

void xlib_log2(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=log(params[0].val->d);
return;
}

void xlib_log10(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=log10(params[0].val->d);
return;
}

void xlib_exp(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=sin(params[0].val->d);
return;
}

void xlib_sqr(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=sqrt(params[0].val->d);
return;
}

void xlib_cbrt(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=cbrt(params[0].val->d);
return;
}

void xlib_pow(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=pow(params[0].val->d,params[1].val->d);
return;
}

void xlib_ceil(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=ceil(params[0].val->d);
return;
}

void xlib_floor(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
returnval->val.d=floor(params[0].val->d);
return;
}

void xlib_rand(int paramcount,vars_t *params,libraryreturnvalue *returnval) {
srand(time(NULL));		/* seed randomizer */

returnval->val.d=rand(params[0].val->i,params[1].val->i);
return;
}

