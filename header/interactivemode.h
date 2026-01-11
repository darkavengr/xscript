/*  XScript Version 0.0.1
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
    along with XScript.  If not, see <https://www.gnu.org/licenses/>.
*/

#define INTERACTIVE_BUFFER_SIZE 65536
#define INITIAL_BUFFERSIZE 1024
#define INTERACTIVE_MODE_FLAG	1
#define LIST_LINE_COUNT 20

void InteractiveMode(void);
int quit_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int continue_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int variables_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int load_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int run_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
void sbreak_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int cbreak_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int lbreak_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
void SwitchToInteractiveModeBuffer(void);
int help_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);
int singlestep_command(int tc,char *tokens[MAX_SIZE][MAX_SIZE]);

#ifndef INTERACTIVEMODE_H
	#define INTERACTIVEMODE_H

	typedef struct {
		char *token[MAX_SIZE];
		struct BLOCKSTATEMENTSAVE *last;
		struct BLOCKSTATEMENTSAVE *next;
	} BLOCKSTATEMENTSAVE;
#endif

