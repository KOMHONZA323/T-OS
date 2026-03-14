#include "libc.h"

extern EFI_SYSTEM_TABLE *ST;

// --- Part 1: <stdio.h> ---
struct FILE { int fd; };
static FILE _stdin = {0}, _stdout = {1}, _stderr = {2};
FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

int remove(const char *filename) { return -1; }
int rename(const char *old, const char *new) { return -1; }
FILE *tmpfile(void) { return NULL; }
char *tmpnam(char *s) { return NULL; }
int fclose(FILE *stream) { return EOF; }
int fflush(FILE *stream) { return 0; }
FILE *fopen(const char *filename, const char *mode) { return NULL; }
FILE *freopen(const char *filename, const char *mode, FILE *stream) { return NULL; }
void setbuf(FILE *stream, char *buf) {}
int setvbuf(FILE *stream, char *buf, int mode, size_t size) { return -1; }

static void putchar_uefi(int c) {
    if (!ST) return;
    wchar_t buf[2] = {(wchar_t)c, 0};
    if (c == '\n') {
        wchar_t r[2] = {L'\r', 0};
        ST->ConOut->OutputString(ST->ConOut, (CHAR16*)r);
    }
    ST->ConOut->OutputString(ST->ConOut, (CHAR16*)buf);
}

int putchar(int c) {
    putchar_uefi(c);
    return c;
}

int getchar(void) {
    if (!ST) return 0;
    EFI_INPUT_KEY key;
    EFI_STATUS status;
    UINTN index;
    ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);
    status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    if (status == EFI_SUCCESS) return (int)key.UnicodeChar;
    return 0;
}

int puts(const char *s) {
    while (*s) putchar(*s++);
    putchar('\n');
    return 0;
}

static void itoa_internal(char *buf, int base, long long d) {
    char *p = buf;
    char *p1, *p2;
    unsigned long long ud = (d < 0 && base == 10) ? -d : d;
    if (base == 10 && d < 0) {
        *p++ = '-';
        buf++;
    }
    do {
        int remainder = ud % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= base);
    *p = 0;
    p1 = buf; p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1; *p1 = *p2; *p2 = tmp;
        p1++; p2--;
    }
}

int vsprintf(char *s, const char *format, va_list arg) {
    char *p = s;
    const char *f = format;
    while (*f) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char *s2 = va_arg(arg, char *);
                if (!s2) s2 = "(null)";
                while (*s2) *p++ = *s2++;
            } else if (*f == 'd') {
                int d = va_arg(arg, int);
                char buf[32]; itoa_internal(buf, 10, d);
                char *b = buf; while (*b) *p++ = *b++;
            } else if (*f == 'x') {
                unsigned int x = va_arg(arg, unsigned int);
                char buf[32]; itoa_internal(buf, 16, x);
                char *b = buf; while (*b) *p++ = *b++;
            } else if (*f == 'c') {
                *p++ = (char)va_arg(arg, int);
            } else if (*f == '%') {
                *p++ = '%';
            }
        } else {
            *p++ = *f;
        }
        f++;
    }
    *p = 0;
    return p - s;
}

int sprintf(char *s, const char *format, ...) {
    va_list args; va_start(args, format);
    int r = vsprintf(s, format, args);
    va_end(args); return r;
}

int printf(const char *format, ...) {
    char buf[1024]; va_list args; va_start(args, format);
    int r = vsprintf(buf, format, args); va_end(args);
    char *p = buf; while (*p) putchar(*p++);
    return r;
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list args; va_start(args, format);
    int r = vprintf(format, args);
    va_end(args); return r;
}

int vprintf(const char *format, va_list arg) {
    char buf[1024]; int r = vsprintf(buf, format, arg);
    char *p = buf; while (*p) putchar(*p++);
    return r;
}

int vfprintf(FILE *stream, const char *format, va_list arg) { return vprintf(format, arg); }
int fscanf(FILE *stream, const char *format, ...) { return 0; }
int scanf(const char *format, ...) { return 0; }
int sscanf(const char *s, const char *format, ...) { return 0; }
int fgetc(FILE *stream) { return (stream == stdin) ? getchar() : EOF; }
char *fgets(char *s, int n, FILE *stream) {
    if (n <= 0) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(stream);
        if (c <= 0 || c == EOF) break;
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = 0;
    return i > 0 ? s : NULL;
}
int fputc(int c, FILE *stream) { return putchar(c); }
int fputs(const char *s, FILE *stream) { while (*s) fputc(*s++, stream); return 0; }
int getc(FILE *stream) { return fgetc(stream); }
char *gets(char *s) { return fgets(s, 1024, stdin); }
int putc(int c, FILE *stream) { return fputc(c, stream); }
int ungetc(int c, FILE *stream) { return EOF; }
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) { return 0; }
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    const char *p = ptr; size_t total = size * nmemb;
    for (size_t i = 0; i < total; i++) fputc(p[i], stream);
    return nmemb;
}
int fgetpos(FILE *stream, fpos_t *pos) { return -1; }
int fseek(FILE *stream, long offset, int whence) { return -1; }
int fsetpos(FILE *stream, const fpos_t *pos) { return -1; }
long ftell(FILE *stream) { return -1; }
void rewind(FILE *stream) {}
void clearerr(FILE *stream) {}
int feof(FILE *stream) { return 0; }
int ferror(FILE *stream) { return 0; }
void perror(const char *s) { if (s) printf("%s: ", s); printf("Error\n"); }

// --- Part 2: <stdlib.h> ---
double atof(const char *nptr) { return 0.0; }
int atoi(const char *nptr) {
    int res = 0, sign = 1;
    while (isspace(*nptr)) nptr++;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') nptr++;
    while (isdigit(*nptr)) res = res * 10 + (*nptr++ - '0');
    return res * sign;
}
long atol(const char *nptr) { return (long)atoi(nptr); }
double strtod(const char *nptr, char **endptr) { if (endptr) *endptr = (char *)nptr; return 0.0; }
long strtol(const char *nptr, char **endptr, int base) { if (endptr) *endptr = (char *)nptr; return 0; }
unsigned long strtoul(const char *nptr, char **endptr, int base) { if (endptr) *endptr = (char *)nptr; return 0; }

static unsigned int next_rand = 1;
int rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (unsigned int)(next_rand / 65536) % 32768;
}
void srand(unsigned int seed) { next_rand = seed; }

void *malloc(size_t size) {
    void *ptr = NULL;
    if (ST) ST->BootServices->AllocatePool(2, size, &ptr);
    return ptr;
}
void free(void *ptr) { if (ST && ptr) ST->BootServices->FreePool(ptr); }
void *calloc(size_t nmemb, size_t size) {
    void *p = malloc(nmemb * size);
    if (p) memset(p, 0, nmemb * size);
    return p;
}
void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, size); // Note: might over-read old buffer
        free(ptr);
    }
    return new_ptr;
}

void abort(void) { while(1); }
int atexit(void (*func)(void)) { return -1; }
void exit(int status) { while(1); }
char *getenv(const char *name) { return NULL; }
int system(const char *string) { return -1; }

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) { return NULL; }
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {}

int abs(int j) { return j < 0 ? -j : j; }
div_t div(int numer, int denom) { div_t r; r.quot = numer / denom; r.rem = numer % denom; return r; }
long labs(long j) { return j < 0 ? -j : j; }
ldiv_t ldiv(long numer, long denom) { ldiv_t r; r.quot = numer / denom; r.rem = numer % denom; return r; }

int mblen(const char *s, size_t n) { return (s && *s) ? 1 : 0; }
int mbtowc(wchar_t *pwc, const char *s, size_t n) { if (s && pwc) *pwc = *s; return 1; }
int wctomb(char *s, wchar_t wchar) { if (s) *s = (char)wchar; return 1; }
size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n) {
    size_t i = 0;
    while (i < n && s[i]) { if (pwcs) pwcs[i] = s[i]; i++; }
    if (i < n && pwcs) pwcs[i] = 0;
    return i;
}
size_t wcstombs(char *s, const wchar_t *pwcs, size_t n) {
    size_t i = 0;
    while (i < n && pwcs[i]) { if (s) s[i] = (char)pwcs[i]; i++; }
    if (i < n && s) s[i] = 0;
    return i;
}

// --- Part 3: <string.h> ---
void *memset(void *s, int c, size_t n) {
    unsigned char *p = s; while (n--) *p++ = (unsigned char)c; return s;
}
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest; const unsigned char *s = src; while (n--) *d++ = *s++; return dest;
}
void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest; const unsigned char *s = src;
    if (d < s) while (n--) *d++ = *s++;
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dest;
}
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) { if (*p1 != *p2) return *p1 - *p2; p1++; p2++; }
    return 0;
}
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = s;
    while (n--) { if (*p == (unsigned char)c) return (void *)p; p++; }
    return NULL;
}
size_t strlen(const char *s) { size_t len = 0; while (s[len]) len++; return len; }
char *strcpy(char *dest, const char *src) {
    char *d = dest; while ((*d++ = *src++)); return dest;
}
char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest; while (n > 0 && (*d++ = *src++)) n--;
    while (n-- > 0) *d++ = 0;
    return dest;
}
char *strcat(char *dest, const char *src) {
    char *d = dest; while (*d) d++; while ((*d++ = *src++)); return dest;
}
char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest; while (*d) d++;
    while (n-- > 0 && (*d++ = *src++));
    *d = 0; return dest;
}
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    while (n-- > 1 && *s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strcoll(const char *s1, const char *s2) { return strcmp(s1, s2); }
size_t strxfrm(char *dest, const char *src, size_t n) { strncpy(dest, src, n); return strlen(src); }
char *strchr(const char *s, int c) {
    while (*s) { if (*s == (char)c) return (char *)s; s++; }
    return (c == 0) ? (char *)s : NULL;
}
char *strrchr(const char *s, int c) {
    char *last = NULL;
    do { if (*s == (char)c) last = (char *)s; } while (*s++);
    return last;
}
size_t strcspn(const char *s, const char *reject) {
    size_t n = 0;
    while (s[n]) {
        const char *r = reject;
        while (*r) { if (s[n] == *r) return n; r++; }
        n++;
    }
    return n;
}
size_t strspn(const char *s, const char *accept) {
    size_t n = 0;
    while (s[n]) {
        const char *a = accept;
        while (*a) { if (s[n] == *a) break; a++; }
        if (!*a) return n;
        n++;
    }
    return n;
}
char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        const char *a = accept;
        while (*a) { if (*s == *a) return (char *)s; a++; }
        s++;
    }
    return NULL;
}
char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack, *n = needle;
            while (*h && *n && *h == *n) { h++; n++; }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}
char *strtok(char *s, const char *delim) { static char *last; if (!s) s = last; if (!s) return NULL; s += strspn(s, delim); if (!*s) return last = NULL; char *tok = s; s += strcspn(s, delim); if (*s) *s++ = 0; last = s; return tok; }
char *strerror(int errnum) { return "Unknown error"; }

// --- Part 4: <math.h> ---
double fabs(double x) { return x < 0 ? -x : x; }
double floor(double x) { long i = (long)x; return (double)(x < i ? i - 1 : i); }
double ceil(double x) { long i = (long)x; return (double)(x > i ? i + 1 : i); }
double sin(double x) { double res = x, term = x, x2 = x * x; for (int i = 3; i < 15; i += 2) { term *= -x2 / (i * (i - 1)); res += term; } return res; }
double cos(double x) { double res = 1.0, term = 1.0, x2 = x * x; for (int i = 2; i < 15; i += 2) { term *= -x2 / (i * (i - 1)); res += term; } return res; }
double sqrt(double x) { if (x < 0) return 0; double res = x / 2.0; if (x > 0) for (int i = 0; i < 10; i++) res = (res + x / res) / 2.0; return res; }
double pow(double x, double y) { if (y == 0) return 1.0; if (y < 0) return 1.0 / pow(x, -y); double res = 1.0; for (int i = 0; i < (int)y; i++) res *= x; return res; }
double acos(double x) { return 0.0; }
double asin(double x) { return 0.0; }
double atan(double x) { return 0.0; }
double atan2(double y, double x) { return 0.0; }
double tan(double x) { return sin(x)/cos(x); }
double cosh(double x) { return 0.0; }
double sinh(double x) { return 0.0; }
double tanh(double x) { return 0.0; }
double exp(double x) { double res = 1.0, term = 1.0; for (int i = 1; i < 15; i++) { term *= x / i; res += term; } return res; }
double log(double x) { return 0.0; }
double log10(double x) { return 0.0; }
double frexp(double x, int *exp) { *exp = 0; return x; }
double ldexp(double x, int exp) { return x * pow(2, exp); }
double modf(double x, double *iptr) { *iptr = floor(x); return x - *iptr; }
double fmod(double x, double y) { return x - y * floor(x / y); }

// --- Part 5: <ctype.h> ---
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int iscntrl(int c) { return (c >= 0 && c <= 31) || c == 127; }
int isgraph(int c) { return c > 32 && c < 127; }
int isprint(int c) { return c >= 32 && c < 127; }
int ispunct(int c) { return isgraph(c) && !isalnum(c); }
int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int toupper(int c) { return islower(c) ? (c - 'a' + 'A') : c; }
int tolower(int c) { return isupper(c) ? (c - 'A' + 'a') : c; }

// --- Part 6: <time.h> ---
time_t time(time_t *timer) {
    if (!ST) return 0;
    EFI_TIME now; ST->RuntimeServices->GetTime(&now, NULL);
    time_t t = now.Year * 31536000 + now.Month * 2592000 + now.Day * 86400 + now.Hour * 3600 + now.Minute * 60 + now.Second;
    if (timer) *timer = t;
    return t;
}
clock_t clock(void) { return (clock_t)time(NULL); }
double difftime(time_t t1, time_t t0) { return (double)(t1 - t0); }
time_t mktime(struct tm *tp) { return 0; }
char *asctime(const struct tm *tp) {
    static char buf[32]; sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", tp->tm_year, tp->tm_mon, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    return buf;
}
char *ctime(const time_t *t) { return asctime(localtime(t)); }
struct tm *gmtime(const time_t *t) { static struct tm r; return &r; }
struct tm *localtime(const time_t *t) { return gmtime(t); }
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tp) { if (max > 0) *s = 0; return 0; }

// --- Part 7: <signal.h> ---
void (*signal(int sig, void (*func)(int)))(int) { return (void *)0; }
int raise(int sig) { return 0; }

// --- Part 8: <setjmp.h> ---
__asm__ (".global setjmp\nsetjmp:\nmov %rbx, (%rcx)\nmov %rbp, 8(%rcx)\nmov %rdi, 16(%rcx)\nmov %rsi, 24(%rcx)\nmov %r12, 32(%rcx)\nmov %r13, 40(%rcx)\nmov %r14, 48(%rcx)\nmov %r15, 56(%rcx)\nlea 8(%rsp), %rdx\nmov %rdx, 64(%rcx)\nmov (%rsp), %rdx\nmov %rdx, 72(%rcx)\nxor %rax, %rax\nret\n");
__asm__ (".global longjmp\nlongjmp:\nmov (%rcx), %rbx\nmov 8(%rcx), %rbp\nmov 16(%rcx), %rdi\nmov 24(%rcx), %rsi\nmov 32(%rcx), %r12\nmov 40(%rcx), %r13\nmov 48(%rcx), %r14\nmov 56(%rcx), %r15\nmov 64(%rcx), %rsp\nmov %rdx, %rax\ntest %rax, %rax\njnz 1f\ninc %rax\n1:\njmp *72(%rcx)\n");

// --- Part 9: <locale.h> ---
char *setlocale(int cat, const char *loc) { return "C"; }
struct lconv *localeconv(void) { static struct lconv lc = { "." }; return &lc; }

// --- Part 10: <wchar.h> ---
size_t wcslen(const wchar_t *s) { size_t len = 0; while (s[len]) len++; return len; }
wchar_t *wcscpy(wchar_t *d, const wchar_t *s) { wchar_t *p = d; while ((*p++ = *s++)); return d; }
wchar_t *wcsncpy(wchar_t *d, const wchar_t *s, size_t n) { wchar_t *p = d; while (n > 0 && (*p++ = *s++)) n--; while (n-- > 0) *p++ = 0; return d; }
int wcscmp(const wchar_t *s1, const wchar_t *s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *s1 - *s2; }
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n) { if (n == 0) return 0; while (n-- > 1 && *s1 && (*s1 == *s2)) { s1++; s2++; } return *s1 - *s2; }
wchar_t *wcscat(wchar_t *d, const wchar_t *s) { wchar_t *p = d; while (*p) p++; while ((*p++ = *s++)); return d; }
wchar_t *wcsncat(wchar_t *d, const wchar_t *s, size_t n) { wchar_t *p = d; while (*p) p++; while (n-- > 0 && (*p++ = *s++)); *p = 0; return d; }
int wcscoll(const wchar_t *s1, const wchar_t *s2) { return wcscmp(s1, s2); }
size_t wcsxfrm(wchar_t *d, const wchar_t *s, size_t n) { wcsncpy(d, s, n); return wcslen(s); }
wchar_t *wcschr(const wchar_t *s, wchar_t c) { while (*s) { if (*s == c) return (wchar_t *)s; s++; } return (c == 0) ? (wchar_t *)s : NULL; }
wchar_t *wcsrchr(const wchar_t *s, wchar_t c) { wchar_t *last = NULL; do { if (*s == c) last = (wchar_t *)s; } while (*s++); return last; }
size_t wcscspn(const wchar_t *s, const wchar_t *r) { size_t n = 0; while (s[n]) { const wchar_t *p = r; while (*p) { if (s[n] == *p) return n; p++; } n++; } return n; }
size_t wcsspn(const wchar_t *s, const wchar_t *a) { size_t n = 0; while (s[n]) { const wchar_t *p = a; while (*p) { if (s[n] == *p) break; p++; } if (!*p) return n; n++; } return n; }
wchar_t *wcspbrk(const wchar_t *s, const wchar_t *a) { while (*s) { const wchar_t *p = a; while (*p) { if (*s == *p) return (wchar_t *)s; p++; } s++; } return NULL; }
wchar_t *wcsstr(const wchar_t *h, const wchar_t *n) { if (!*n) return (wchar_t *)h; for (; *h; h++) { if (*h == *n) { const wchar_t *hh = h, *nn = n; while (*hh && *nn && *hh == *nn) { hh++; nn++; } if (!*nn) return (wchar_t *)h; } } return NULL; }
wchar_t *wcstok(wchar_t *s, const wchar_t *d, wchar_t **p) { if (!s) s = *p; if (!s) return NULL; s += wcsspn(s, d); if (!*s) return *p = NULL; wchar_t *tok = s; s += wcscspn(s, d); if (*s) *s++ = 0; *p = s; return tok; }
size_t wcsftime(wchar_t *s, size_t max, const wchar_t *f, const struct tm *t) { if (max > 0) *s = 0; return 0; }

int wprintf(const wchar_t *format, ...) { if (!ST) return 0; ST->ConOut->OutputString(ST->ConOut, (CHAR16*)format); return 0; }
int fwprintf(FILE *s, const wchar_t *f, ...) { return 0; }
int swprintf(wchar_t *s, size_t n, const wchar_t *f, ...) { if (n > 0) *s = 0; return 0; }
int vfwprintf(FILE *s, const wchar_t *f, va_list a) { return 0; }
int vswprintf(wchar_t *s, size_t n, const wchar_t *f, va_list a) { if (n > 0) *s = 0; return 0; }
int vwprintf(const wchar_t *f, va_list a) { return 0; }
int wscanf(const wchar_t *f, ...) { return 0; }
int fwscanf(FILE *s, const wchar_t *f, ...) { return 0; }
int swscanf(const wchar_t *s, const wchar_t *f, ...) { return 0; }
int vfwscanf(FILE *s, const wchar_t *f, va_list a) { return 0; }
int vswscanf(const wchar_t *s, const wchar_t *f, va_list a) { return 0; }
int vwscanf(const wchar_t *f, va_list a) { return 0; }

wint_t fgetwc(FILE *s) { return WEOF; }
wchar_t *fgetws(wchar_t *s, int n, FILE *st) { if (n > 0) *s = 0; return NULL; }
wint_t fputwc(wchar_t c, FILE *s) { return WEOF; }
int fputws(const wchar_t *s, FILE *st) { return -1; }
int fwide(FILE *s, int m) { return 0; }
wint_t getwc(FILE *s) { return fgetwc(s); }
wint_t getwchar(void) { return fgetwc(stdin); }
wint_t putwc(wchar_t c, FILE *s) { return fputwc(c, s); }
wint_t putwchar(wchar_t c) { return fputwc(c, stdout); }
wint_t ungetwc(wint_t c, FILE *s) { return WEOF; }
double wcstod(const wchar_t *n, wchar_t **e) { if (e) *e = (wchar_t *)n; return 0.0; }
long wcstol(const wchar_t *n, wchar_t **e, int b) { if (e) *e = (wchar_t *)n; return 0; }
unsigned long wcstoul(const wchar_t *n, wchar_t **e, int b) { if (e) *e = (wchar_t *)n; return 0; }
wint_t btowc(int c) { return (c == EOF) ? WEOF : (wchar_t)c; }
int wctob(wint_t c) { return (c == WEOF) ? EOF : (int)c; }
int mbsinit(const mbstate_t *ps) { return 1; }
size_t mbrlen(const char *s, size_t n, mbstate_t *ps) { return (s && *s) ? 1 : 0; }
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps) { if (s && pwc) *pwc = *s; return 1; }
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps) { if (s) *s = (char)wc; return 1; }
size_t mbsrtowcs(wchar_t *d, const char **s, size_t l, mbstate_t *ps) { return mbstowcs(d, *s, l); }
size_t wcsrtombs(char *d, const wchar_t **s, size_t l, mbstate_t *ps) { return wcstombs(d, *s, l); }

// --- Part 11: <wctype.h> ---
int iswalnum(wint_t wc) { return isalnum((int)wc); }
int iswalpha(wint_t wc) { return isalpha((int)wc); }
int iswcntrl(wint_t wc) { return iscntrl((int)wc); }
int iswdigit(wint_t wc) { return isdigit((int)wc); }
int iswgraph(wint_t wc) { return isgraph((int)wc); }
int iswlower(wint_t wc) { return islower((int)wc); }
int iswprint(wint_t wc) { return isprint((int)wc); }
int iswpunct(wint_t wc) { return ispunct((int)wc); }
int iswspace(wint_t wc) { return isspace((int)wc); }
int iswupper(wint_t wc) { return isupper((int)wc); }
int iswxdigit(wint_t wc) { return isxdigit((int)wc); }
wctype_t wctype(const char *p) { return 0; }
int iswctype(wint_t wc, wctype_t d) { return 0; }
wint_t towlower(wint_t wc) { return tolower((int)wc); }
wint_t towupper(wint_t wc) { return toupper((int)wc); }
wctrans_t wctrans(const char *p) { return 0; }
wint_t towctrans(wint_t wc, wctrans_t d) { return wc; }
