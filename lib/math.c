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

/* IMPORTANT: XScript passed ALL parameters as POINTERS (see module.c) */

int xlib_abs(int *num) {
return(abs(*num));
}

double xlib_arctan(double *num) {
return(atan(*num));
}

double xlib_cos(double *num) {
return(cos(*num));
}

double xlib_tan(double *num) {
return(tan(*num));
}

double xlib_sin(double *num) {
return(sin(*num));
}

double xlib_log(double *num) {
return(log(*num));
}

double xlib_log2(double *num) {
return(log2(*num));
}

double xlib_log10(double *num) {
return(log10(*num));
}

double xlib_exp(double *num) {
return(exp(*num));
}

double xlib_sqr(double *num) {
return(sqr(*num));
}

double xlib_cubrt(double *num) {
return(cbrt(*num));
}

double xlib_pow(double *num) {
return(pow(*num));
}

double xlib_ceil(double *num) {
return(ceil(*num));
}

double xlib_floor(double *num) {
return(floor(*num));
}

int xlib_rand(int startnum,int endnum) {
srand(time(NULL));		/* seed randomizer */

return(rand() % endnum+startnum);
}

