/**
 *
 *    _    __        _      __                           
 *   | |  / /____   (_)____/ /_      __ ____ _ _____ ___ 
 *   | | / // __ \ / // __  /| | /| / // __ `// ___// _ \
 *   | |/ // /_/ // // /_/ / | |/ |/ // /_/ // /   /  __/
 *   |___/ \____//_/ \__,_/  |__/|__/ \__,_//_/    \___/ 
 *                                                       
 *  Copyright (©) Voidware 2018.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 * 
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 * 
 *  contact@voidware.com
 */

#ifndef __libc_h__
#define __libc_h__

#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END };
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif 

#include <stddef.h> // size_t
#include <stdarg.h> // va_list

#ifdef _WIN32
typedef int smint;
typedef unsigned int usmint;
#else
// if we need to, make a very small int
typedef char smint;
typedef unsigned char usmint;
#endif


EXTERN_C_BEGIN

#include "libcbase.h"

#define DIM(_x)         (sizeof(_x)/sizeof((_x)[0]))

typedef struct _StreamRec
{
    usmint              h_;
    usmint              ioIndex_;
    int                 fpos_;
    unsigned char*      buf_;
    unsigned char*      pos_;
    unsigned char*      end_;
    usmint              flags_;
} StreamRec;

typedef StreamRec FILE;

    //extern StreamRec ioTable[];

#define stdout (&ioTable[1])
#define stdin  (&ioTable[0])
#define EOF    (-1)

//void *   memcpy(void *, const void *, size_t);
//void *   memset(void *, unsigned char, size_t); 
void*    memsetInt(void* ptr, int v, size_t n);
char *   strcpy(char *, const char *);
char *   strcat(char *, const char *);
//smint    strcmp(const char *, const char *);
size_t   strlen(const char *);
//void *   memmove(void *, const void *, size_t);

char*    strchr(const char*, smint);
smint    stricmp(const char*, const char*);
char*    strncat(char*, const char*, size_t);
//smint    strncmp(const char*, const char*, size_t);
smint    strnicmp(const char* s1, const char* s2, size_t n);
char*    strncpy(char*, const char*, size_t);
char*    strrchr(const char*, smint);
size_t   strspn(const char*, const char*);
char*    strstr(const char*, const char*);
char*    strtok(char*, const char*);

/* file functions */
smint    printf(const char *, ...);
smint    sprintf(char *, const char *, ...);
smint    fprintf(FILE *, const char *, ...);
smint    vprintf(const char *, va_list);
smint    vfprintf(FILE *, const char *, va_list);
smint    vsprintf(char *, const char *, va_list);
int      fputc(int, FILE *);
int      fputs(const char *, FILE *);
int      putc(int, FILE *);
int      putchar(int);
void     flush();
smint    fflush(FILE *);
size_t   fwrite(const void *, size_t, size_t, FILE *);
smint    fscanf(FILE *, const char *, ...);
int      getchar(void);
int      getc(FILE *);

int      fclose(FILE *);
int      fgetc(FILE *);
FILE*    fopen(const char *, const char *);
size_t   fread(void *, size_t, size_t, FILE *);
FILE*    freopen(const char *, const char *, FILE *);
int      fseek(FILE *, long, int);
long     ftell(FILE *);
char*    gets(char *);
void     rewind(FILE *);
smint    scanf(const char *, ...);
void     setbuf(FILE *, char *);
int      sscanf(const char *, const char *, ...);
int      ungetc(int, FILE *);


/* ctype functions */
#undef _tolower
#undef _toupper
#undef isalpha
#undef isupper
#undef islower
#undef isdigit
#undef isspace
#undef ispunct
#undef isalnum
#undef isprint

#define _tolower(_c)    ( (_c)-'A'+'a' )
#define _toupper(_c)    ( (_c)-'a'+'A' )

#define isalpha(_c)  (isupper(_c) || islower(_c))
#define isupper(_c)  (((_c) >= 'A') & ((_c) <= 'Z'))
#define islower(_c)  (((_c) >= 'a') & ((_c) <= 'z'))
#define isdigit(_c)  (((_c) >= '0') & ((_c) <= '9'))
#define isspace(_c)  (((_c) == ' ') | ((_c) == '\t') | ((_c) == '\n'))
#define ispunct(_c)
#define isalnum(_c)  (isalpha(_c) || isdigit(_c))
#define isprint(_c)  (((_c) >= 0x20) & ((_c) <= 127))

/* malloc */
void*               malloc(size_t);
void                free(void*);
void*               realloc(void* p, size_t amt);

/* call this to initialise libc */
void            libcInit();

EXTERN_C_END

#endif // __libc_h__
