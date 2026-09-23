#ifndef __SYLIB_H_
#define __SYLIB_H_

#include <stdio.h>

char get_char();
int get_int();
void get_string(char *buffer, int max_len);

void put_int(int a);
void put_char(char a);
void put_string(char *str);

#endif
