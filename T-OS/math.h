#ifndef _MATH_H_
#define _MATH_H_

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"

// Trigonometric
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);
double cos(double x);
double sin(double x);
double tan(double x);

// Hyperbolic
double cosh(double x);
double sinh(double x);
double tanh(double x);

// Exponential/Logarithmic
double exp(double x);
double frexp(double value, int *exp);
double ldexp(double x, int exp);
double log(double x);
double log10(double x);
double modf(double value, double *iptr);

// Power/Absolute/Rounding
double pow(double x, double y);
double sqrt(double x);
double ceil(double x);
double fabs(double x);
double floor(double x);
double fmod(double x, double y);

#endif
