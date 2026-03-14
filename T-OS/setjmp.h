#ifndef _SETJMP_H_
#define _SETJMP_H_

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"

typedef void* jmp_buf[16]; // opaque type for setjmp

// Note: setjmp is a macro: int setjmp(jmp_buf env);
#define setjmp(env) (0) // stub
void longjmp(jmp_buf env, int val);

#endif
