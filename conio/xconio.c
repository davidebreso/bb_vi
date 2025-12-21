/* vi: set sw=4 ts=4: */
/*
 * Low level terminal interface routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 * Copyright (C) 2006 Rob Landley
 * Copyright (C) 2006 Denys Vlasenko
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include "libbb.h"
#include "platform.h"
#include "terminal.h"

#include <conio.h>
#include <graph.h>
#include <i86.h>


struct term T;
static char clrbuf[MAX_SCR_COLS];

int FAST_FUNC safe_read_key(int timeout)
{
	int r;

	if ((r = getch()) == 0) {
		r = getch() << 8;
	}
	return r;
}

void FAST_FUNC show_help(void)
{
	puts("These features are available:"
#if ENABLE_FEATURE_VI_SEARCH
	"\n\tPattern searches with / and ?"
#endif
#if ENABLE_FEATURE_VI_DOT_CMD
	"\n\tLast command repeat with ."
#endif
#if ENABLE_FEATURE_VI_YANKMARK
	"\n\tLine marking with 'x"
	"\n\tNamed buffers with \"x"
#endif
#if ENABLE_FEATURE_VI_READONLY
	//not implemented: "\n\tReadonly if vi is called as \"view\""
	//redundant: usage text says this too: "\n\tReadonly with -R command line arg"
#endif
#if ENABLE_FEATURE_VI_SET
	"\n\tSome colon mode commands with :"
#endif
#if ENABLE_FEATURE_VI_SETOPTS
	"\n\tSettable options with \":set\""
#endif
#if ENABLE_FEATURE_VI_USE_SIGNALS
	"\n\tSignal catching- ^C"
	"\n\tJob suspend and resume with ^Z"
#endif
#if ENABLE_FEATURE_VI_WIN_RESIZE
	"\n\tAdapt to window re-sizes"
#endif
	);
}

int FAST_FUNC bb_putchar(int ch) {
	char c = (char)ch;
    _outmem(&c, 1);
    return ch;

}

void FAST_FUNC write1(const char *out)
{
	_outtext(out);
}

void FAST_FUNC write2(const char *out, size_t n)
{
	_outmem(out, n);
}

int FAST_FUNC query_screen_dimensions(void)
{
	struct videoconfig vc;

	_getvideoconfig(&vc);
	rows = vc.numtextrows;
	columns = vc.numtextcols;

	return 0;
}

// sleep for 'h' 1/100 seconds, return 1/0 if stdin is (ready for read)/(not ready)
int FAST_FUNC mysleep(int hund)
{
	delay(hund*10);
	return 0;
}

//----- Set terminal attributes --------------------------------
void FAST_FUNC rawmode(void)
{
    _wrapon(_GWRAPOFF);
}

void FAST_FUNC cookmode(void)
{
    _wrapon(_GWRAPON);
}

//----- Terminal Drawing ---------------------------------------
// The terminal is made up of 'rows' line of 'columns' columns.
// classically this would be 24 x 80.
//  screen coordinates
//  0,0     ...     0,79
//  1,0     ...     1,79
//  .       ...     .
//  .       ...     .
//  22,0    ...     22,79
//  23,0    ...     23,79   <- status line

//----- Move the cursor to row x col (count from 0, not 1) -------
void FAST_FUNC place_cursor(int row, int col)
{
	if (row < 0) row = 0;
	if (row >= rows) row = rows - 1;
	if (col < 0) col = 0;
	if (col >= columns) col = columns - 1;

	_settextposition(row + 1, col + 1);
}


//----- Erase from cursor to end of line -----------------------
void FAST_FUNC clear_to_eol(void)
{
	struct rccoord old_pos = _gettextposition();
	int len = columns - old_pos.col + 1;

	memset(clrbuf, ' ', len);
	_outmem(clrbuf, len);
	_settextposition(old_pos.row, old_pos.col);
}


//----- Go to upper left corner and erase screen ---------------
void FAST_FUNC home_and_clear_to_eos(void)
{
	_clearscreen(_GWINDOW);
}

void FAST_FUNC go_bottom_and_clear_to_eol(void)
{
	place_cursor(rows - 1, 0);
	clear_to_eol();
}

void FAST_FUNC insert_line(void)
{
	_scrolltextwindow(_GSCROLLUP);
	place_cursor(rows - 1, 0);
}

//----- Start standout mode ------------------------------------
void FAST_FUNC standout_start(void)
{
	_setbkcolor(7);
	_settextcolor(0);
}

//----- End standout mode --------------------------------------
void FAST_FUNC standout_end(void)
{
	_setbkcolor(0);
	_settextcolor(7);
}

//----- Ring a bell --------------------------------------------
void FAST_FUNC bell(void)
{
	sound(440);
	delay(100);  /* delay for 1/2 second */
	nosound();
}

//----- Initialize terminal ------------------------------------
void FAST_FUNC init_term(void)
{
	struct videoconfig vc;

	rawmode();
	_getvideoconfig(&vc);
	rows = vc.numtextrows;
	columns = vc.numtextcols;
	pages = vc.numvideopages;
}

void FAST_FUNC alternate_screen_buffer_start(void)
{
	// "Save cursor, use alternate screen buffer, clear screen"
	if (pages > 1) {
		int newpage;
		oldpage = _getvisualpage();
		oldpos = _gettextposition();
		newpage = (oldpage == 0 ? 1 : 0);
		_setvisualpage(newpage);
		_setactivepage(newpage);
		_clearscreen(_GCLEARSCREEN);
	}
}

void FAST_FUNC alternate_screen_buffer_end(void)
{
	// "Use normal screen buffer, restore cursor"
	if (pages > 1){
		_setvisualpage(oldpage);
		_setactivepage(oldpage);
		_settextposition(oldpos.row, oldpos.col);
	}
}
