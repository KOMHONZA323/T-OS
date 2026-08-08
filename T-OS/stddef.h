#ifndef _STDDEF_H_
#define _STDDEF_H_

typedef unsigned long size_t;
typedef long ptrdiff_t;
#define NULL ((void*)0)

// stdlib.h declares mbtowc()/wcstombs() and so needs wchar_t, but pulling in
// all of wchar.h for one typedef would be circular. C puts it here anyway.
//
// T-OS is built with -fshort-wchar to match UEFI's CHAR16, so wchar_t is 16
// bits wide; libc.h and wchar.h defer to this definition.
#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#endif

#endif
