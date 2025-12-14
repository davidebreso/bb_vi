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

/* fdprintf is more readable, we used it before dprintf was standardized */
#include <unistd.h>
#define fdprintf dprintf

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
#define HAVE_CLEARENV 1
#define HAVE_FDATASYNC 1
#define HAVE_DPRINTF 1
#define HAVE_MEMRCHR 1
#define HAVE_MKDTEMP 1
#define HAVE_TTYNAME_R 1
#define HAVE_PTSNAME_R 1
#define HAVE_SETBIT 1
#define HAVE_SIGHANDLER_T 1
#define HAVE_STPCPY 1
#define HAVE_MEMPCPY 1
#define HAVE_STRCASESTR 1
#define HAVE_STRCHRNUL 1
#define HAVE_STRSEP 1
#define HAVE_STRSIGNAL 1
#define HAVE_STRVERSCMP 1
#define HAVE_VASPRINTF 1
#define HAVE_USLEEP 1
#define HAVE_UNLOCKED_STDIO 1
#define HAVE_UNLOCKED_LINE_OPS 1
#define HAVE_GETLINE 1
#define HAVE_XTABS 1
#define HAVE_MNTENT_H 1
#define HAVE_NET_ETHERNET_H 1
#define HAVE_SYS_STATFS_H 1
#define HAVE_PRINTF_PERCENTM 1

#if defined(__UCLIBC__)
#if UCLIBC_VERSION < KERNEL_VERSION(0, 9, 32)
#undef HAVE_STRVERSCMP
#endif
#if UCLIBC_VERSION >= KERNEL_VERSION(0, 9, 30)
#ifndef __UCLIBC_SUSV3_LEGACY__
#undef HAVE_USLEEP
#endif
#endif
#endif

#if defined(__WATCOMC__)
#undef HAVE_DPRINTF
#undef HAVE_GETLINE
#undef HAVE_MEMRCHR
#undef HAVE_MKDTEMP
#undef HAVE_SETBIT
#undef HAVE_STPCPY
#undef HAVE_STRCASESTR
#undef HAVE_STRCHRNUL
#undef HAVE_STRSEP
#undef HAVE_STRSIGNAL
#undef HAVE_STRVERSCMP
#undef HAVE_VASPRINTF
#undef HAVE_UNLOCKED_STDIO
#undef HAVE_UNLOCKED_LINE_OPS
#undef HAVE_NET_ETHERNET_H
#endif

#if defined(__CYGWIN__)
#undef HAVE_CLEARENV
#undef HAVE_FDPRINTF
#undef HAVE_MEMRCHR
#undef HAVE_PTSNAME_R
#undef HAVE_STRVERSCMP
#undef HAVE_UNLOCKED_LINE_OPS
#endif

/* These BSD-derived OSes share many similarities */
#if (defined __digital__ && defined __unix__) || defined __APPLE__ || defined __OpenBSD__ || defined __NetBSD__
#undef HAVE_CLEARENV
#undef HAVE_FDATASYNC
#undef HAVE_GETLINE
#undef HAVE_MNTENT_H
#undef HAVE_PTSNAME_R
#undef HAVE_SYS_STATFS_H
#undef HAVE_SIGHANDLER_T
#undef HAVE_STRVERSCMP
#undef HAVE_XTABS
#undef HAVE_DPRINTF
#undef HAVE_UNLOCKED_STDIO
#undef HAVE_UNLOCKED_LINE_OPS
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
/* users say mempcpy is not present in FreeBSD 9.x */
#undef HAVE_MEMPCPY
#undef HAVE_CLEARENV
#undef HAVE_FDATASYNC
#undef HAVE_MNTENT_H
#undef HAVE_PTSNAME_R
#undef HAVE_SYS_STATFS_H
#undef HAVE_SIGHANDLER_T
#undef HAVE_STRVERSCMP
#undef HAVE_XTABS
#undef HAVE_UNLOCKED_LINE_OPS
#undef HAVE_PRINTF_PERCENTM
#include <osreldate.h>
#if __FreeBSD_version < 1000029
#undef HAVE_STRCHRNUL /* FreeBSD added strchrnul() between 1000028 and 1000029 */
#endif
#endif

#if defined(__NetBSD__)
#define HAVE_GETLINE 1 /* Recent NetBSD versions have getline() */
#endif

#if defined(__digital__) && defined(__unix__)
#undef HAVE_STPCPY
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#if __ANDROID_API__ < 8
/* ANDROID < 8 has no [f]dprintf at all */
#undef HAVE_DPRINTF
#elif __ANDROID_API__ < 21
/* ANDROID < 21 has fdprintf */
#define dprintf fdprintf
#else
/* ANDROID >= 21 has standard dprintf */
#endif
#if __ANDROID_API__ < 21
#undef HAVE_TTYNAME_R
#undef HAVE_GETLINE
#undef HAVE_STPCPY
#endif
#undef HAVE_MEMPCPY
#undef HAVE_STRCHRNUL
#undef HAVE_STRVERSCMP
#undef HAVE_UNLOCKED_LINE_OPS
#undef HAVE_NET_ETHERNET_H
#undef HAVE_PRINTF_PERCENTM
#endif

/*
 * Now, define prototypes for all the functions defined in platform.c
 * These must come after all the HAVE_* macros are defined (or not)
 */

#ifndef HAVE_DPRINTF
extern int dprintf(int fd, const char *format, ...);
#endif

#ifndef HAVE_MEMRCHR
extern void *memrchr(const void *s, int c, size_t n) FAST_FUNC;
#endif

#ifndef HAVE_MKDTEMP
extern char *mkdtemp(char *template) FAST_FUNC;
#endif

#ifndef HAVE_TTYNAME_R
#define ttyname_r bb_ttyname_r
extern int ttyname_r(int fd, char *buf, size_t buflen);
#endif

#ifndef HAVE_SETBIT
#define setbit(a, b) ((a)[(b) >> 3] |= 1 << ((b)&7))
#define clrbit(a, b) ((a)[(b) >> 3] &= ~(1 << ((b)&7)))
#endif

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#ifndef HAVE_STPCPY
extern char *stpcpy(char *p, const char *to_add) FAST_FUNC;
#endif

#ifndef HAVE_MEMPCPY
#include <string.h>
/* In case we are wrong about !HAVE_MEMPCPY, and toolchain _does_ have
 * mempcpy(), avoid colliding with it:
 */
#define mempcpy bb__mempcpy
static ALWAYS_INLINE void *mempcpy(void *dest, const void *src, size_t len) {
    return memcpy(dest, src, len) + len;
}
#endif

#ifndef HAVE_STRCASESTR
extern char *strcasestr(const char *s, const char *pattern) FAST_FUNC;
#endif

#ifndef HAVE_STRCHRNUL
extern char *strchrnul(const char *s, int c) FAST_FUNC;
#endif

#ifndef HAVE_STRSEP
extern char *strsep(char **stringp, const char *delim) FAST_FUNC;
#endif

#ifndef HAVE_STRSIGNAL
/* Not exactly the same: instead of "Stopped" it shows "STOP" etc */
#define strsignal(sig) get_signame(sig)
#endif

#ifndef HAVE_USLEEP
extern int usleep(unsigned) FAST_FUNC;
#endif

#ifndef HAVE_VASPRINTF
extern int vasprintf(char **string_ptr, const char *format, va_list p) FAST_FUNC;
#endif

#ifndef HAVE_GETLINE
#include <stdio.h>     /* for FILE */
#include <sys/types.h> /* size_t */
extern ssize_t getline(char **lineptr, size_t *n, FILE *stream) FAST_FUNC;
#endif

#endif
