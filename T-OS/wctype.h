#ifndef _WCTYPE_H_
#define _WCTYPE_H_

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"

typedef int wint_t;
typedef int wctype_t;
typedef int wctrans_t;

// Classification
int iswalnum(wint_t wc);
int iswalpha(wint_t wc);
int iswcntrl(wint_t wc);
int iswctype(wint_t wc, wctype_t desc);
int iswdigit(wint_t wc);
int iswgraph(wint_t wc);
int iswlower(wint_t wc);
int iswprint(wint_t wc);
int iswpunct(wint_t wc);
int iswspace(wint_t wc);
int iswupper(wint_t wc);
int iswxdigit(wint_t wc);

// Mapping (Case Conversion)
wint_t towlower(wint_t wc);
wint_t towupper(wint_t wc);
wint_t towctrans(wint_t wc, wctrans_t desc);

// Extensibility
wctype_t wctype(const char *property);
wctrans_t wctrans(const char *property);

#endif
