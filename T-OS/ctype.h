#ifndef _CTYPE_H_
#define _CTYPE_H_

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"

// Checking
int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isisdigit(int c); // typo fixed below
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);

// Conversion
int tolower(int c);
int toupper(int c);

#endif
