/* vi: set sw=4 ts=4: */
/*
 * Busybox terminal interface header file
 *
 * Based in part on code from sash, Copyright (c) 1999 by David I. Bell
 * Permission has been granted to redistribute this code under GPL.
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#ifndef TERMINAL_H
#define TERMINAL_H 1

#include "platform.h"
#include "autoconf.h"

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>

enum {
	MAX_TABSTOP = 32, // sanity limit
	// User input len. Need not be extra big.
	// Lines in file being edited *can* be bigger than this.
	MAX_INPUT_LEN = 128,
	// Sanity limits. We have only one buffer of this size.
	MAX_SCR_COLS = CONFIG_FEATURE_VI_MAX_LEN,
	MAX_SCR_ROWS = CONFIG_FEATURE_VI_MAX_LEN,
};

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

struct term {
 	int rows, columns;	 // the terminal screen is this size
#if ENABLE_FEATURE_VI_ASK_TERMINAL
	int get_rowcol_error;
#endif
	struct termios term_orig; // remember what the cooked mode was
	// Should be just enough to hold a key sequence,
	// but CRASHME mode uses it as generated command buffer too
#if ENABLE_FEATURE_VI_CRASHME
	char readbuffer[128];
#else
	char readbuffer[KEYCODE_BUFFER_SIZE];
#endif
};

extern struct term T;
#define rows                    (T.rows               )
#define columns                 (T.columns            )
#define term_orig               (T.term_orig          )
#define readbuffer              (T.readbuffer         )

#define isbackspace(c)	((c) == term_orig.c_cc[VERASE] || (c) == 8 || (c) == 127)

// xtermios.c
int64_t read_key(int fd, char *buffer, int timeout) FAST_FUNC;
int64_t safe_read_key(int fd, char *buffer, int timeout) FAST_FUNC;
void show_help(void) FAST_FUNC;
void write1(const char *out) FAST_FUNC;
int query_screen_dimensions(void) FAST_FUNC;
int mysleep(int hund) FAST_FUNC;
void rawmode(void) FAST_FUNC;
void cookmode(void) FAST_FUNC;
void place_cursor(int row, int col) FAST_FUNC;
void clear_to_eol(void) FAST_FUNC;
void home_and_clear_to_eos(void) FAST_FUNC;
void go_bottom_and_clear_to_eol(void) FAST_FUNC;
void standout_start(void) FAST_FUNC;
void standout_end(void) FAST_FUNC;
void bell(void) FAST_FUNC;
void init_term(void) FAST_FUNC;
void alternate_screen_buffer_start(void) FAST_FUNC;
void alternate_screen_buffer_end(void) FAST_FUNC;

#endif
