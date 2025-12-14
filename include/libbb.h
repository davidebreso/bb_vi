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
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>

/* Some useful definitions */
#undef FALSE
#define FALSE ((int)0)
#undef TRUE
#define TRUE ((int)1)
#undef SKIP
#define SKIP ((int)2)

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

/* "Keycodes" that report an escape sequence.
 * We use something which fits into signed char,
 * yet doesn't represent any valid Unicode character.
 * Also, -1 is reserved for error indication and we don't use it. */
enum {
    KEYCODE_UP = -2,
    KEYCODE_DOWN = -3,
    KEYCODE_RIGHT = -4,
    KEYCODE_LEFT = -5,
    KEYCODE_HOME = -6,
    KEYCODE_END = -7,
    KEYCODE_INSERT = -8,
    KEYCODE_DELETE = -9,
    KEYCODE_PAGEUP = -10,
    KEYCODE_PAGEDOWN = -11,
    KEYCODE_BACKSPACE = -12, /* Used only if Alt/Ctrl/Shifted */
    KEYCODE_D = -13,         /* Used only if Alted */
#if 0
	KEYCODE_FUN1      = ,
	KEYCODE_FUN2      = ,
	KEYCODE_FUN3      = ,
	KEYCODE_FUN4      = ,
	KEYCODE_FUN5      = ,
	KEYCODE_FUN6      = ,
	KEYCODE_FUN7      = ,
	KEYCODE_FUN8      = ,
	KEYCODE_FUN9      = ,
	KEYCODE_FUN10     = ,
	KEYCODE_FUN11     = ,
	KEYCODE_FUN12     = ,
#endif
    /* ^^^^^ Be sure that last defined value is small enough.
	 * Current read_key() code allows going up to -32 (0xfff..fffe0).
	 * This gives three upper bits in LSB to play with:
	 * KEYCODE_foo values are 0xfff..fffXX, lowest XX bits are: scavvvvv,
	 * s=0 if SHIFT, c=0 if CTRL, a=0 if ALT,
	 * vvvvv bits are the same for same key regardless of "shift bits".
	 */
    // KEYCODE_SHIFT_...   = KEYCODE_...   & ~0x80,
    KEYCODE_CTRL_RIGHT = KEYCODE_RIGHT & ~0x40,
    KEYCODE_CTRL_LEFT = KEYCODE_LEFT & ~0x40,
    KEYCODE_ALT_RIGHT = KEYCODE_RIGHT & ~0x20,
    KEYCODE_ALT_LEFT = KEYCODE_LEFT & ~0x20,
    KEYCODE_ALT_BACKSPACE = KEYCODE_BACKSPACE & ~0x20,
    KEYCODE_ALT_D = KEYCODE_D & ~0x20,

    KEYCODE_CURSOR_POS = -0x100, /* 0xfff..fff00 */
    /* How long is the longest ESC sequence we know?
	 * We want it big enough to be able to contain
	 * cursor position sequence "ESC [ 9999 ; 9999 R"
	 */
    KEYCODE_BUFFER_SIZE = 16
};

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
#define TERMIOS_CLEAR_ISIG (1 << 0)
#define TERMIOS_RAW_CRNL_INPUT (1 << 1)
#define TERMIOS_RAW_CRNL_OUTPUT (1 << 2)
#define TERMIOS_RAW_CRNL (TERMIOS_RAW_CRNL_INPUT | TERMIOS_RAW_CRNL_OUTPUT)
#define TERMIOS_RAW_INPUT (1 << 3)
ssize_t safe_write(int fd, const void *buf, size_t count) FAST_FUNC;
ssize_t full_write(int fd, const void *buf, size_t len) FAST_FUNC;
int get_terminal_width_height(int fd, int *width, int *height) FAST_FUNC;
int get_terminal_width(int fd) FAST_FUNC;
int tcsetattr_stdin_TCSANOW(const struct termios *tp) FAST_FUNC;
int get_termios_and_make_raw(int fd, struct termios *newterm, struct termios *oldterm, int flags) FAST_FUNC;
int set_termios_to_raw(int fd, struct termios *oldterm, int flags) FAST_FUNC;
int safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout) FAST_FUNC;

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
int bb_putchar(int ch) FAST_FUNC;
int fputs_stdout(const char *s) FAST_FUNC;
char *xasprintf(const char *format, ...) __attribute__ ((format(printf, 1, 2))) FAST_FUNC RETURNS_MALLOC;

// read.c
ssize_t safe_read(int fd, void *buf, size_t count) FAST_FUNC;
ssize_t full_read(int fd, void *buf, size_t len) FAST_FUNC;

// read_key.c
int64_t read_key(int fd, char *buffer, int timeout) FAST_FUNC;
int64_t safe_read_key(int fd, char *buffer, int timeout) FAST_FUNC;
char *last_char_is(const char *s, int c) FAST_FUNC;
char *skip_whitespace(const char *s) FAST_FUNC;
char *skip_non_whitespace(const char *s) FAST_FUNC;

// safe_strncpy.c
void overlapping_strcpy(char *dst, const char *src) FAST_FUNC;

/* Concatenate path and filename to new allocated buffer.
 * Add "/" only as needed (no duplicate "//" are produced).
 * If path is NULL, it is assumed to be "/".
 * filename should not be NULL. */
char *concat_path_file(const char *path, const char *filename) FAST_FUNC;

int index_in_strings(const char *strings, const char *key) FAST_FUNC;

extern void *xmalloc_open_read_close(const char *filename, size_t *maxsz_p) FAST_FUNC RETURNS_MALLOC;

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
