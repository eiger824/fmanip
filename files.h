/*
 * Filename:		files.h
 *
 * Author:			Santiago Pagola
 * Brief:			Operations on files
 * Last modified:	tis 20 mar 2018 11:59:39 CET
*/

#ifndef FILES_H_
#define FILES_H_

#include <stdio.h>

#include "defs.h"

void subst_bytes(const char * fname, int val1, int val2, int debug);
u8_t get_byte_value(const char* fname, int pos, int debug);
int set_byte_value(const char* fname, int pos, int to, u8_t val, int debug);
int dump(const char* fname, int mark, int range, int byteval);
size_t file_size (FILE * fp);

#endif /* FILES_H_ */
