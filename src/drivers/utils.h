#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(char *dest, char val, int len);
void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void delay(int count);

#endif
