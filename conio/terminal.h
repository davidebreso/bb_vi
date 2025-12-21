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

#include <graph.h>

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
    KEYCODE_UP = 0x4800,
    KEYCODE_DOWN = 0x5000,
    KEYCODE_RIGHT = 0x4D00,
    KEYCODE_LEFT = 0x4B00,
    KEYCODE_HOME = 0x4700,
    KEYCODE_END = 0x4F00,
    KEYCODE_INSERT = 0x5200,
    KEYCODE_DELETE = 0x5300,
    KEYCODE_PAGEUP = 0x4900,
    KEYCODE_PAGEDOWN = 0x5100,
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
    KEYCODE_CTRL_RIGHT = 0x7400,
    KEYCODE_CTRL_LEFT = 0x7300,
    KEYCODE_ALT_RIGHT = 0x9D00,
    KEYCODE_ALT_LEFT = 0x9B00,
    KEYCODE_ALT_BACKSPACE = 0x7F00,
    KEYCODE_ALT_D = 0x2000,

    /* How long is the longest ESC sequence we know?
	 * We want it big enough to be able to contain
	 * cursor position sequence "ESC [ 9999 ; 9999 R"
	 */
    KEYCODE_BUFFER_SIZE = 16
};

struct term {
 	int rows, columns;	 	    // the terminal screen is this size
 	int pages;						    // number of video pages
 	int oldpage;					    // active page at startup
 	struct rccoord oldpos;		// cursor position at startup
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
#define pages                   (T.pages            )
#define oldpage                 (T.oldpage            )
#define oldpos                  (T.oldpos            )
#define readbuffer              (T.readbuffer         )

#define isbackspace(c)	((c) == 8 || (c) == 127)

// xconio.c
int safe_read_key(int timeout) FAST_FUNC;
void show_help(void) FAST_FUNC;
int bb_putchar(int ch) FAST_FUNC;
void write1(const char *out) FAST_FUNC;
void write2(const char *out, size_t n) FAST_FUNC;
int query_screen_dimensions(void) FAST_FUNC;
int mysleep(int hund) FAST_FUNC;
void rawmode(void) FAST_FUNC;
void cookmode(void) FAST_FUNC;
void place_cursor(int row, int col) FAST_FUNC;
void clear_to_eol(void) FAST_FUNC;
void home_and_clear_to_eos(void) FAST_FUNC;
void go_bottom_and_clear_to_eol(void) FAST_FUNC;
void insert_line(void) FAST_FUNC;
void standout_start(void) FAST_FUNC;
void standout_end(void) FAST_FUNC;
void bell(void) FAST_FUNC;
void init_term(void) FAST_FUNC;
void alternate_screen_buffer_start(void) FAST_FUNC;
void alternate_screen_buffer_end(void) FAST_FUNC;

#endif
