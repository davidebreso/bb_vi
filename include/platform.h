/* vi: set sw=4 ts=4: */
/*
 * Copyright 2006, Bernhard Reutner-Fischer
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
#ifndef BB_PLATFORM_H
#define BB_PLATFORM_H 1

/* Convenience macros to test the version of gcc. */
#undef __GNUC_PREREQ
#if defined __GNUC__ && defined __GNUC_MINOR__
#define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define __GNUC_PREREQ(maj, min) 0
#endif

#if !__GNUC_PREREQ(2, 7)
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#undef inline
#if defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
/* it's a keyword */
#elif __GNUC_PREREQ(2, 7)
#define inline __inline__
#else
#define inline
#endif

#define UNUSED_PARAM __attribute__((__unused__))
#define NORETURN __attribute__((__noreturn__))

/* "The malloc attribute is used to tell the compiler that a function
 * may be treated as if any non-NULL pointer it returns cannot alias
 * any other pointer valid when the function returns. This will often
 * improve optimization. Standard functions with this property include
 * malloc and calloc. realloc-like functions have this property as long
 * as the old pointer is never referred to (including comparing it
 * to the new pointer) after the function returns a non-NULL value."
 */
#define RETURNS_MALLOC __attribute__((malloc))

/* __NO_INLINE__: some gcc's do not honor inlining! :( */
#if __GNUC_PREREQ(3, 0) && !defined(__NO_INLINE__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
/* I've seen a toolchain where I needed __noinline__ instead of noinline */
#define NOINLINE __attribute__((__noinline__))
#if !ENABLE_WERROR
#define DEPRECATED __attribute__((__deprecated__))
#define UNUSED_PARAM_RESULT __attribute__((warn_unused_result))
#else
#define DEPRECATED
#define UNUSED_PARAM_RESULT
#endif
#else
#define ALWAYS_INLINE inline
#define NOINLINE
#define DEPRECATED
#define UNUSED_PARAM_RESULT
#endif

/* FAST_FUNC is a qualifier which (possibly) makes function call faster
 * and/or smaller by using modified ABI. It is usually only needed
 * on non-static, busybox internal functions. Recent versions of gcc
 * optimize statics automatically. FAST_FUNC on static is required
 * only if you need to match a function pointer's type.
 * FAST_FUNC may not work well with -flto so allow user to disable this.
 * (-DFAST_FUNC= )
 */
#ifndef FAST_FUNC
#if __GNUC_PREREQ(3, 0) && defined(i386)
/* stdcall makes callee to pop arguments from stack, not caller */
#define FAST_FUNC __attribute__((regparm(3), stdcall))
/* #elif ... - add your favorite arch today! */
#else
#define FAST_FUNC
#endif
#endif

/* gcc-2.95 had no va_copy but only __va_copy. */
#if !__GNUC_PREREQ(3, 0)
#include <stdarg.h>
#if !defined va_copy && defined __va_copy
#define va_copy(d, s) __va_copy((d), (s))
#endif
#endif

#include <limits.h>

/* ---- Size-saving "small" ints (arch-dependent) ----------- */

#if defined(i386) || defined(__x86_64__) || defined(__mips__) || defined(__cris__)
/* add other arches which benefit from this... */
typedef signed char smallint;
typedef unsigned char smalluint;
#else
/* for arches where byte accesses generate larger code: */
typedef int smallint;
typedef unsigned smalluint;
#endif

/* ISO C Standard:  7.16  Boolean type and values  <stdbool.h> */
#if (defined __digital__ && defined __unix__)
/* old system without (proper) C99 support */
#define bool smalluint
#else
/* modern system, so use it */
#include <stdbool.h>
#endif

/*----- Kernel versioning ------------------------------------*/

#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))

#ifdef __UCLIBC__
#define UCLIBC_VERSION KERNEL_VERSION(__UCLIBC_MAJOR__, __UCLIBC_MINOR__, __UCLIBC_SUBLEVEL__)
#else
#define UCLIBC_VERSION 0
#endif

#include <unistd.h>

/* Useful for defeating gcc's alignment of "char message[]"-like data */
#if !defined(__s390__)
/* on s390[x], non-word-aligned data accesses require larger code */
#define ALIGN1 __attribute__((aligned(1)))
#define ALIGN2 __attribute__((aligned(2)))
#define ALIGN4 __attribute__((aligned(4)))
#else
/* Arches which MUST have 2 or 4 byte alignment for everything are here */
#define ALIGN1
#define ALIGN2
#define ALIGN4
#endif
#define ALIGN8 __attribute__((aligned(8)))
#define ALIGN_PTR __attribute__((aligned(sizeof(void *))))

/* ---- Who misses what? ------------------------------------ */

/* Assume all these functions and header files exist by default.
 * Platforms where it is not true will #undef them below.
 */
#define HAVE_MEMRCHR 1
#define HAVE_STRCHRNUL 1
#define HAVE_STRNDUP 1
#define HAVE_VASPRINTF 1
#define HAVE_PRINTF_PERCENTM 1

#if defined(__WATCOMC__)
#undef HAVE_MEMRCHR
#undef HAVE_STRCHRNUL
#undef HAVE_STRNDUP
#undef HAVE_VASPRINTF
#undef HAVE_PRINTF_PERCENTM
#include <strings.h>
#include <io.h>
#include <tchar.h>
#define uintptr_t unsigned int
/*
 * on DOS, use _tell to get the current position in the file,
 * since the byte count returned by write does not include extra CR added to text files
 */
#define ftruncate(fd, n) _chsize(fd, _tell(fd))
#define strnlen _strncnt
#define getuid() (0)
#endif

#if defined(__CYGWIN__)
#undef HAVE_MEMRCHR
#endif

/* These BSD-derived OSes share many similarities */
#if (defined __digital__ && defined __unix__) || defined __APPLE__ || defined __OpenBSD__ || defined __NetBSD__
#undef HAVE_PRINTF_PERCENTM
#endif

#if defined(__dietlibc__)
#undef HAVE_STRCHRNUL
#endif

#if defined(__APPLE__)
#undef HAVE_STRCHRNUL
#undef HAVE_MEMRCHR
#endif

#if defined(__FreeBSD__)
#undef HAVE_PRINTF_PERCENTM
#include <osreldate.h>
#if __FreeBSD_version < 1000029
#undef HAVE_STRCHRNUL /* FreeBSD added strchrnul() between 1000028 and 1000029 */
#endif
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#undef HAVE_STRCHRNUL
#undef HAVE_PRINTF_PERCENTM
#endif

/*
 * Now, define prototypes for all the functions defined in platform.c
 * These must come after all the HAVE_* macros are defined (or not)
 */

#ifndef HAVE_MEMRCHR
extern void *memrchr(const void *s, int c, size_t n) FAST_FUNC;
#endif

#ifndef HAVE_STRCHRNUL
extern char *strchrnul(const char *s, int c) FAST_FUNC;
#endif

#ifndef HAVE_STRNDUP
extern char *strndup(const char *s, size_t n) FAST_FUNC;
#endif

#ifndef HAVE_VASPRINTF
extern int vasprintf(char **string_ptr, const char *format, va_list p) FAST_FUNC;
#endif

#endif
