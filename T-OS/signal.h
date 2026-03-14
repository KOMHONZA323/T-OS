#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"

// Signal Handling
void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);

#endif
