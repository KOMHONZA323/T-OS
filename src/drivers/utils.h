#ifndef UTILS_H
#define UTILS_H

void memory_copy(char *source, char *dest, int n_bytes);
void memory_set(char *dest, char val, int len);
int strlen(const char *s);
void itoa(int n, char str[]);
void reverse(char s[]);
void delay(int count);

#endif
