/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * Based in part on code from sash, Copyright (c) 1999 by David I. Bell
 * Permission has been granted to redistribute this code under GPL.
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#ifndef LIBBB_H
#define LIBBB_H 1

#include "platform.h"
#include "autoconf.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

/* Some useful definitions */
#undef FALSE
#define FALSE ((int)0)
#undef TRUE
#define TRUE ((int)1)

/* Macros for min/max.  */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define ARRAY_SIZE(x) ((int)(sizeof(x) / sizeof((x)[0])))

#define vi_main main
#define applet_name "vi"
#define BB_VER "1.37.0"

#ifdef HAVE_PRINTF_PERCENTM
#define STRERROR_FMT "%m"
#define STRERROR_ERRNO /*nothing*/
#else
#define STRERROR_FMT "%s"
#define STRERROR_ERRNO , strerror(errno)
#endif

// verror_msg.c
enum {
    LOGMODE_NONE = 0,
    LOGMODE_STDIO = (1 << 0),
    LOGMODE_SYSLOG = (1 << 1) * ENABLE_FEATURE_SYSLOG,
    LOGMODE_BOTH = LOGMODE_SYSLOG + LOGMODE_STDIO,
};
void bb_verror_msg(const char *s, va_list p, const char *strerr) FAST_FUNC;
void bb_error_msg(const char *s, ...) FAST_FUNC;
void bb_error_msg_and_die(const char *s, ...) FAST_FUNC;
void bb_simple_error_msg(const char *s) FAST_FUNC;
void bb_simple_error_msg_and_die(const char *s) FAST_FUNC;
void xfunc_die(void) FAST_FUNC;

// xfuncs.c
ssize_t safe_write(int fd, const void *buf, size_t count) FAST_FUNC;
ssize_t full_write(int fd, const void *buf, size_t len) FAST_FUNC;

// xfuncs_printf.c
#ifdef DMALLOC
#include <dmalloc.h>
#endif
void *xmalloc(size_t size) FAST_FUNC RETURNS_MALLOC;
void *xzalloc(size_t size) FAST_FUNC RETURNS_MALLOC;
void *xrealloc(void *old, size_t size) FAST_FUNC;
char *xstrdup(const char *s) FAST_FUNC RETURNS_MALLOC;
char *xstrndup(const char *s, int n) FAST_FUNC RETURNS_MALLOC;
int fflush_all(void) FAST_FUNC;
char *xasprintf(const char *format, ...) __attribute__ ((format(printf, 1, 2))) FAST_FUNC RETURNS_MALLOC;

// read.c
ssize_t safe_read(int fd, void *buf, size_t count) FAST_FUNC;
ssize_t full_read(int fd, void *buf, size_t len) FAST_FUNC;

// xstring.c
char *last_char_is(const char *s, int c) FAST_FUNC;
char *skip_whitespace(const char *s) FAST_FUNC;
char *skip_non_whitespace(const char *s) FAST_FUNC;

// safe_strncpy.c
void overlapping_strcpy(char *dst, const char *src) FAST_FUNC;

// concat_path_file.c
/* Concatenate path and filename to new allocated buffer.
 * Add "/" only as needed (no duplicate "//" are produced).
 * If path is NULL, it is assumed to be "/".
 * filename should not be NULL. */
char *concat_path_file(const char *path, const char *filename) FAST_FUNC;

// compare_string_array.c
int index_in_strings(const char *strings, const char *key) FAST_FUNC;

// read_printf.c
void *xmalloc_open_read_close(const char *filename, size_t *maxsz_p) FAST_FUNC RETURNS_MALLOC;

// llist.c
/* Having next pointer as a first member allows easy creation
 * of "llist-compatible" structs, and using llist_FOO functions
 * on them.
 */
typedef struct llist_t {
	struct llist_t *link;
	char *data;
} llist_t;
void llist_add_to_end(llist_t **list_head, void *data) FAST_FUNC;
void *llist_pop(llist_t **elm) FAST_FUNC;

#endif
