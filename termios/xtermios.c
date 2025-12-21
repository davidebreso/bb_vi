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

// VT102 ESC sequences.
// See "Xterm Control Sequences"
// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
#define ESC "\033"
// Inverse/Normal text
#define ESC_BOLD_TEXT ESC"[7m"
#define ESC_NORM_TEXT ESC"[m"
// Bell
#define ESC_BELL "\007"
// Clear-to-end-of-line
#define ESC_CLEAR2EOL ESC"[K"
// Clear-to-end-of-screen.
// (We use default param here.
// Full sequence is "ESC [ <num> J",
// <num> is 0/1/2 = "erase below/above/all".)
#define ESC_CLEAR2EOS          ESC"[J"
// Cursor to given coordinate (1,1: top left)
#define ESC_SET_CURSOR_POS     ESC"[%u;%uH"
#define ESC_SET_CURSOR_TOPLEFT ESC"[H"
//UNUSED
//// Cursor up and down
//#define ESC_CURSOR_UP   ESC"[A"
//#define ESC_CURSOR_DOWN "\n"

#define TERMIOS_CLEAR_ISIG (1 << 0)
#define TERMIOS_RAW_CRNL_INPUT (1 << 1)
#define TERMIOS_RAW_CRNL_OUTPUT (1 << 2)
#define TERMIOS_RAW_CRNL (TERMIOS_RAW_CRNL_INPUT | TERMIOS_RAW_CRNL_OUTPUT)
#define TERMIOS_RAW_INPUT (1 << 3)

struct term T;

static int wh_helper(int value, int def_val, const char *env_name, int *err) {
    /* Envvars override even if "value" from ioctl is valid (>0).
	 * Rationale: it's impossible to guess what user wants.
	 * For example: "man CMD | ...": should "man" format output
	 * to stdout's width? stdin's width? /dev/tty's width? 80 chars?
	 * We _cant_ know it. If "..." saves text for e.g. email,
	 * then it's probably 80 chars.
	 * If "..." is, say, "grep -v DISCARD | $PAGER", then user
	 * would prefer his tty's width to be used!
	 *
	 * Since we don't know, at least allow user to do this:
	 * "COLUMNS=80 man CMD | ..."
	 */
    char *s = getenv(env_name);
    if (s) {
        value = atoi(s);
        /* If LINES/COLUMNS are set, pretend that there is
		 * no error getting w/h, this prevents some ugly
		 * cursor tricks by our callers */
        *err = 0;
    }

    if (value <= 1 || value >= 30000)
        value = def_val;
    return value;
}

/* It is perfectly ok to pass in a NULL for either width or for
 * height, in which case that value will not be set.  */
static int FAST_FUNC get_terminal_width_height(int fd, int *width, int *height) {
    struct winsize win;
    int err;
    int close_me = -1;

    if (fd == -1) {
        if (isatty(STDOUT_FILENO))
            fd = STDOUT_FILENO;
        else if (isatty(STDERR_FILENO))
            fd = STDERR_FILENO;
        else if (isatty(STDIN_FILENO))
            fd = STDIN_FILENO;
        else
            close_me = fd = open("/dev/tty", O_RDONLY);
    }

    win.ws_row = 0;
    win.ws_col = 0;
    /* I've seen ioctl returning 0, but row/col is (still?) 0.
	 * We treat that as an error too.  */
    err = ioctl(fd, TIOCGWINSZ, &win) != 0 || win.ws_row == 0;
    if (height)
        *height = wh_helper(win.ws_row, 24, "LINES", &err);
    if (width)
        *width = wh_helper(win.ws_col, 80, "COLUMNS", &err);

    if (close_me >= 0)
        close(close_me);

    return err;
}

static int FAST_FUNC tcsetattr_stdin_TCSANOW(const struct termios *tp) {
    return tcsetattr(STDIN_FILENO, TCSANOW, tp);
}

static int FAST_FUNC get_termios_and_make_raw(int fd, struct termios *newterm, struct termios *oldterm, int flags) {
    //TODO: slattach, shell read might be adapted to use this too: grep for "tcsetattr", "[VTIME] = 0"
    int r;

    memset(oldterm, 0, sizeof(*oldterm)); /* paranoia */
    r = tcgetattr(fd, oldterm);
    *newterm = *oldterm;

    /* Turn off buffered input (ICANON)
	 * Turn off echoing (ECHO)
	 * and separate echoing of newline (ECHONL, normally off anyway)
	 */
    newterm->c_lflag &= ~(ICANON | ECHO | ECHONL);
    if (flags & TERMIOS_CLEAR_ISIG) {
        /* dont recognize INT/QUIT/SUSP chars */
        newterm->c_lflag &= ~ISIG;
    }
    /* reads will block only if < 1 char is available */
    newterm->c_cc[VMIN] = 1;
    /* no timeout (reads block forever) */
    newterm->c_cc[VTIME] = 0;
    /* IXON, IXOFF, and IXANY:
 * IXOFF=1: sw flow control is enabled on input queue:
 * tty transmits a STOP char when input queue is close to full
 * and transmits a START char when input queue is nearly empty.
 * IXON=1: sw flow control is enabled on output queue:
 * tty will stop sending if STOP char is received,
 * and resume sending if START is received, or if any char
 * is received and IXANY=1.
 */
    if (flags & TERMIOS_RAW_CRNL_INPUT) {
        /* IXON=0: XON/XOFF chars are treated as normal chars (why we do this?) */
        /* dont convert CR to NL on input */
        newterm->c_iflag &= ~(IXON | ICRNL);
    }
    if (flags & TERMIOS_RAW_CRNL_OUTPUT) {
        /* dont convert NL to CR+NL on output */
        newterm->c_oflag &= ~(ONLCR);
        /* Maybe clear more c_oflag bits? Usually, only OPOST and ONLCR are set.
		 * OPOST  Enable output processing (reqd for OLCUC and *NL* bits to work)
		 * OLCUC  Map lowercase characters to uppercase on output.
		 * OCRNL  Map CR to NL on output.
		 * ONOCR  Don't output CR at column 0.
		 * ONLRET Don't output CR.
		 */
    }
    if (flags & TERMIOS_RAW_INPUT) {
#ifndef IMAXBEL
#define IMAXBEL 0
#endif
#ifndef IUCLC
#define IUCLC 0
#endif
#ifndef IXANY
#define IXANY 0
#endif
        /* IXOFF=0: disable sending XON/XOFF if input buf is full
		 * IXON=0: input XON/XOFF chars are not special
		 * BRKINT=0: dont send SIGINT on break
		 * IMAXBEL=0: dont echo BEL on input line too long
		 * INLCR,ICRNL,IUCLC: dont convert anything on input
		 */
        newterm->c_iflag &= ~(IXOFF | IXON | IXANY | BRKINT | INLCR | ICRNL | IUCLC | IMAXBEL);
    }
    return r;
}

static int FAST_FUNC set_termios_to_raw(int fd, struct termios *oldterm, int flags) {
    struct termios newterm;

    get_termios_and_make_raw(fd, &newterm, oldterm, flags);
    return tcsetattr(fd, TCSANOW, &newterm);
}

/* Wrapper which restarts poll on EINTR or ENOMEM.
 * On other errors does perror("poll") and returns.
 * Warning! May take longer than timeout_ms to return! */
int FAST_FUNC safe_poll(struct pollfd *ufds, nfds_t nfds, int timeout) {
    while (1) {
        int n = poll(ufds, nfds, timeout);
        if (n >= 0)
            return n;
        /* Make sure we inch towards completion */
        if (timeout > 0)
            timeout--;
        /* E.g. strace causes poll to return this */
        if (errno == EINTR)
            continue;
        /* Kernel is very low on memory. Retry. */
        /* I doubt many callers would handle this correctly! */
        if (errno == ENOMEM)
            continue;
        bb_simple_error_msg("poll");
        return n;
    }
}

int64_t FAST_FUNC read_key(int fd, char *buffer, int timeout) {
    struct pollfd pfd;
    const char *seq;
    int n;

    /* Known escape sequences for cursor and function keys.
	 * See "Xterm Control Sequences"
	 * http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
	 * Array should be sorted from shortest to longest.
	 */
    static const char esccmds[] ALIGN1 = {
        '\x7f' | 0x80,
        KEYCODE_ALT_BACKSPACE,
        '\b' | 0x80,
        KEYCODE_ALT_BACKSPACE,
        'd' | 0x80,
        KEYCODE_ALT_D,
        /* lineedit mimics bash: Alt-f and Alt-b are forward/backward
	 * word jumps. We cheat here and make them return ALT_LEFT/RIGHT
	 * keycodes. This way, lineedit need no special code to handle them.
	 * If we'll need to distinguish them, introduce new ALT_F/B keycodes,
	 * and update lineedit to react to them.
	 */
        'f' | 0x80,
        KEYCODE_ALT_RIGHT,
        'b' | 0x80,
        KEYCODE_ALT_LEFT,
        'O',
        'A' | 0x80,
        KEYCODE_UP,
        'O',
        'B' | 0x80,
        KEYCODE_DOWN,
        'O',
        'C' | 0x80,
        KEYCODE_RIGHT,
        'O',
        'D' | 0x80,
        KEYCODE_LEFT,
        'O',
        'H' | 0x80,
        KEYCODE_HOME,
        'O',
        'F' | 0x80,
        KEYCODE_END,
#if 0
		'O','P'        |0x80,KEYCODE_FUN1    ,
		/* [ESC] ESC O [2] P - [Alt-][Shift-]F1 */
		/* ESC [ O 1 ; 2 P - Shift-F1 */
		/* ESC [ O 1 ; 3 P - Alt-F1 */
		/* ESC [ O 1 ; 4 P - Alt-Shift-F1 */
		/* ESC [ O 1 ; 5 P - Ctrl-F1 */
		/* ESC [ O 1 ; 6 P - Ctrl-Shift-F1 */
		'O','Q'        |0x80,KEYCODE_FUN2    ,
		'O','R'        |0x80,KEYCODE_FUN3    ,
		'O','S'        |0x80,KEYCODE_FUN4    ,
#endif
        '[',
        'A' | 0x80,
        KEYCODE_UP,
        '[',
        'B' | 0x80,
        KEYCODE_DOWN,
        '[',
        'C' | 0x80,
        KEYCODE_RIGHT,
        '[',
        'D' | 0x80,
        KEYCODE_LEFT,
        /* ESC [ 1 ; 2 x, where x = A/B/C/D: Shift-<arrow> */
        /* ESC [ 1 ; 3 x, where x = A/B/C/D: Alt-<arrow> - implemented below */
        /* ESC [ 1 ; 4 x, where x = A/B/C/D: Alt-Shift-<arrow> */
        /* ESC [ 1 ; 5 x, where x = A/B/C/D: Ctrl-<arrow> - implemented below */
        /* ESC [ 1 ; 6 x, where x = A/B/C/D: Ctrl-Shift-<arrow> */
        /* ESC [ 1 ; 7 x, where x = A/B/C/D: Ctrl-Alt-<arrow> */
        /* ESC [ 1 ; 8 x, where x = A/B/C/D: Ctrl-Alt-Shift-<arrow> */
        '[',
        'H' | 0x80,
        KEYCODE_HOME, /* xterm */
        '[',
        'F' | 0x80,
        KEYCODE_END, /* xterm */
        /* [ESC] ESC [ [2] H - [Alt-][Shift-]Home (End similarly?) */
        /* '[','Z'        |0x80,KEYCODE_SHIFT_TAB, */
        '[',
        '1',
        '~' | 0x80,
        KEYCODE_HOME, /* vt100? linux vt? or what? */
        '[',
        '2',
        '~' | 0x80,
        KEYCODE_INSERT,
        /* ESC [ 2 ; 3 ~ - Alt-Insert */
        '[',
        '3',
        '~' | 0x80,
        KEYCODE_DELETE,
        /* [ESC] ESC [ 3 [;2] ~ - [Alt-][Shift-]Delete */
        /* ESC [ 3 ; 3 ~ - Alt-Delete */
        /* ESC [ 3 ; 5 ~ - Ctrl-Delete */
        '[',
        '4',
        '~' | 0x80,
        KEYCODE_END, /* vt100? linux vt? or what? */
        '[',
        '5',
        '~' | 0x80,
        KEYCODE_PAGEUP,
        /* ESC [ 5 ; 3 ~ - Alt-PgUp */
        /* ESC [ 5 ; 5 ~ - Ctrl-PgUp */
        /* ESC [ 5 ; 7 ~ - Ctrl-Alt-PgUp */
        '[',
        '6',
        '~' | 0x80,
        KEYCODE_PAGEDOWN,
        '[',
        '7',
        '~' | 0x80,
        KEYCODE_HOME, /* vt100? linux vt? or what? */
        '[',
        '8',
        '~' | 0x80,
        KEYCODE_END, /* vt100? linux vt? or what? */
#if 0
		'[','1','1','~'|0x80,KEYCODE_FUN1    , /* old xterm, deprecated by ESC O P */
		'[','1','2','~'|0x80,KEYCODE_FUN2    , /* old xterm... */
		'[','1','3','~'|0x80,KEYCODE_FUN3    , /* old xterm... */
		'[','1','4','~'|0x80,KEYCODE_FUN4    , /* old xterm... */
		'[','1','5','~'|0x80,KEYCODE_FUN5    ,
		/* [ESC] ESC [ 1 5 [;2] ~ - [Alt-][Shift-]F5 */
		'[','1','7','~'|0x80,KEYCODE_FUN6    ,
		'[','1','8','~'|0x80,KEYCODE_FUN7    ,
		'[','1','9','~'|0x80,KEYCODE_FUN8    ,
		'[','2','0','~'|0x80,KEYCODE_FUN9    ,
		'[','2','1','~'|0x80,KEYCODE_FUN10   ,
		'[','2','3','~'|0x80,KEYCODE_FUN11   ,
		'[','2','4','~'|0x80,KEYCODE_FUN12   ,
		/* ESC [ 2 4 ; 2 ~ - Shift-F12 */
		/* ESC [ 2 4 ; 3 ~ - Alt-F12 */
		/* ESC [ 2 4 ; 4 ~ - Alt-Shift-F12 */
		/* ESC [ 2 4 ; 5 ~ - Ctrl-F12 */
		/* ESC [ 2 4 ; 6 ~ - Ctrl-Shift-F12 */
#endif
        /* '[','1',';','5','A' |0x80,KEYCODE_CTRL_UP   , - unused */
        /* '[','1',';','5','B' |0x80,KEYCODE_CTRL_DOWN , - unused */
        '[',
        '1',
        ';',
        '5',
        'C' | 0x80,
        KEYCODE_CTRL_RIGHT,
        '[',
        '1',
        ';',
        '5',
        'D' | 0x80,
        KEYCODE_CTRL_LEFT,
        /* '[','1',';','3','A' |0x80,KEYCODE_ALT_UP    , - unused */
        /* '[','1',';','3','B' |0x80,KEYCODE_ALT_DOWN  , - unused */
        '[',
        '1',
        ';',
        '3',
        'C' | 0x80,
        KEYCODE_ALT_RIGHT,
        '[',
        '1',
        ';',
        '3',
        'D' | 0x80,
        KEYCODE_ALT_LEFT,
        /* '[','3',';','3','~' |0x80,KEYCODE_ALT_DELETE, - unused */
        0
    };

    pfd.fd = fd;
    pfd.events = POLLIN;

    buffer++; /* saved chars counter is in buffer[-1] now */

start_over:
    errno = 0;
    n = (unsigned char)buffer[-1];
    if (n == 0) {
        /* If no data, wait for input.
		 * If requested, wait TIMEOUT ms. TIMEOUT = -1 is useful
		 * if fd can be in non-blocking mode.
		 */
        if (timeout >= -1) {
            if (safe_poll(&pfd, 1, timeout) == 0) {
                /* Timed out */
                errno = EAGAIN;
                return -1;
            }
        }
        /* It is tempting to read more than one byte here,
		 * but it breaks pasting. Example: at shell prompt,
		 * user presses "c","a","t" and then pastes "\nline\n".
		 * When we were reading 3 bytes here, we were eating
		 * "li" too, and cat was getting wrong input.
		 */
        n = safe_read(fd, buffer, 1);
        if (n <= 0)
            return -1;
    }

    {
        unsigned char c = buffer[0];
        n--;
        if (n)
            memmove(buffer, buffer + 1, n);
        /* Only ESC starts ESC sequences */
        if (c != 27) {
            buffer[-1] = n;
            return c;
        }
    }

    /* Loop through known ESC sequences */
    seq = esccmds;
    while (*seq != '\0') {
        /* n - position in sequence we did not read yet */
        int i = 0; /* position in sequence to compare */

        /* Loop through chars in this sequence */
        while (1) {
            /* So far escape sequence matched up to [i-1] */
            if (n <= i) {
                /* Need more chars, read another one if it wouldn't block.
				 * Note that escape sequences come in as a unit,
				 * so if we block for long it's not really an escape sequence.
				 * Timeout is needed to reconnect escape sequences
				 * split up by transmission over a serial console. */
                if (safe_poll(&pfd, 1, 50) == 0) {
                    /* No more data!
					 * Array is sorted from shortest to longest,
					 * we can't match anything later in array -
					 * anything later is longer than this seq.
					 * Break out of both loops. */
                    goto got_all;
                }
                errno = 0;
                if (safe_read(fd, buffer + n, 1) <= 0) {
                    /* If EAGAIN, then fd is O_NONBLOCK and poll lied:
					 * in fact, there is no data. */
                    if (errno != EAGAIN) {
                        /* otherwise: it's EOF/error */
                        buffer[-1] = 0;
                        return -1;
                    }
                    goto got_all;
                }
                n++;
            }
            if (buffer[i] != (seq[i] & 0x7f)) {
                /* This seq doesn't match, go to next */
                seq += i;
                /* Forward to last char */
                while (!(*seq & 0x80))
                    seq++;
                /* Skip it and the keycode which follows */
                seq += 2;
                break;
            }
            if (seq[i] & 0x80) {
                /* Entire seq matched */
                n = 0;
                /* n -= i; memmove(...);
				 * would be more correct,
				 * but we never read ahead that much,
				 * and n == i here. */
                buffer[-1] = 0;
                return (signed char)seq[i + 1];
            }
            i++;
        }
    }
    /* We did not find matching sequence.
	 * We possibly read and stored more input in buffer[] by now.
	 * n = bytes read. Try to read more until we time out.
	 */
    while (n < KEYCODE_BUFFER_SIZE - 1) { /* 1 for count byte at buffer[-1] */
        if (safe_poll(&pfd, 1, 50) == 0) {
            /* No more data! */
            break;
        }
        errno = 0;
        if (safe_read(fd, buffer + n, 1) <= 0) {
            /* If EAGAIN, then fd is O_NONBLOCK and poll lied:
			 * in fact, there is no data. */
            if (errno != EAGAIN) {
                /* otherwise: it's EOF/error */
                buffer[-1] = 0;
                return -1;
            }
            break;
        }
        n++;
        /* Try to decipher "ESC [ NNN ; NNN R" sequence */
        if ((ENABLE_FEATURE_EDITING_ASK_TERMINAL || ENABLE_FEATURE_VI_ASK_TERMINAL ||
             ENABLE_FEATURE_LESS_ASK_TERMINAL) &&
            n >= 5 && buffer[0] == '[' && buffer[n - 1] == 'R' && isdigit(buffer[1])) {
            char *end;
            unsigned long row, col;

            row = strtoul(buffer + 1, &end, 10);
            if (*end != ';' || !isdigit(end[1]))
                continue;
            col = strtoul(end + 1, &end, 10);
            if (*end != 'R')
                continue;
            if (row < 1 || col < 1 || (row | col) > 0x7fff)
                continue;

            buffer[-1] = 0;
            /* Pack into "1 <row15bits> <col16bits>" 32-bit sequence */
            row |= ((unsigned)(-1) << 15);
            col |= (row << 16);
            /* Return it in high-order word */
            return ((int64_t)col << 32) | (uint32_t)KEYCODE_CURSOR_POS;
        }
    }
got_all:

    if (n <= 1) {
        /* Alt-x is usually returned as ESC x.
		 * Report ESC, x is remembered for the next call.
		 */
        buffer[-1] = n;
        return 27;
    }

    /* We were doing "buffer[-1] = n; return c;" here, but this results
	 * in unknown key sequences being interpreted as ESC + garbage.
	 * This was not useful. Pretend there was no key pressed,
	 * go and wait for a new keypress:
	 */
    buffer[-1] = 0;
    goto start_over;
}

int64_t FAST_FUNC safe_read_key(int timeout)
{
	int64_t r;
	do {
		/* errno = 0; - read_key does this itself */
		r = read_key(STDIN_FILENO, readbuffer, timeout);
	} while (errno == EINTR);
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
    return putchar(ch);
}

void FAST_FUNC write1(const char *out)
{
	fputs(out, stdout);
}

void FAST_FUNC write2(const char *out, size_t n)
{
	fwrite(out, n, 1, stdout);
}

#if ENABLE_FEATURE_VI_WIN_RESIZE
int FAST_FUNC query_screen_dimensions(void)
{
	int err = get_terminal_width_height(STDIN_FILENO, &columns, &rows);
	if (rows > MAX_SCR_ROWS)
		rows = MAX_SCR_ROWS;
	if (columns > MAX_SCR_COLS)
		columns = MAX_SCR_COLS;
	return err;
}
#else
int FAST_FUNCT query_screen_dimensions(void)
{
	return 0;
}
#endif

// sleep for 'h' 1/100 seconds, return 1/0 if stdin is (ready for read)/(not ready)
int FAST_FUNC mysleep(int hund)
{
	struct pollfd pfd[1];

	if (hund != 0)
		fflush_all();

	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	return safe_poll(pfd, 1, hund*10) > 0;
}

//----- Set terminal attributes --------------------------------
void FAST_FUNC rawmode(void)
{
	// no TERMIOS_CLEAR_ISIG: leave ISIG on - allow signals
	set_termios_to_raw(STDIN_FILENO, &term_orig, TERMIOS_RAW_CRNL);
}

void FAST_FUNC cookmode(void)
{
	fflush_all();
	tcsetattr_stdin_TCSANOW(&term_orig);
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
	char cm1[sizeof(ESC_SET_CURSOR_POS) + sizeof(int)*3 * 2];

	if (row < 0) row = 0;
	if (row >= rows) row = rows - 1;
	if (col < 0) col = 0;
	if (col >= columns) col = columns - 1;

	sprintf(cm1, ESC_SET_CURSOR_POS, row + 1, col + 1);
	write1(cm1);
}

//----- Erase from cursor to end of line -----------------------
void FAST_FUNC clear_to_eol(void)
{
	write1(ESC_CLEAR2EOL);
}

//----- Go to upper left corner and erase screen ---------------
void FAST_FUNC home_and_clear_to_eos(void)
{
	write1(ESC_SET_CURSOR_TOPLEFT ESC_CLEAR2EOS);
}

void FAST_FUNC go_bottom_and_clear_to_eol(void)
{
	place_cursor(rows - 1, 0);
	clear_to_eol();
}

void FAST_FUNC insert_line(void)
{
	write1("\r\n");
}

//----- Start standout mode ------------------------------------
void FAST_FUNC standout_start(void)
{
	write1(ESC_BOLD_TEXT);
}

//----- End standout mode --------------------------------------
void FAST_FUNC standout_end(void)
{
	write1(ESC_NORM_TEXT);
}

//----- Ring a bell --------------------------------------------
void FAST_FUNC bell(void)
{
	write1(ESC_BELL);
}

//----- Initialize terminal ------------------------------------
void FAST_FUNC init_term(void)
{
	rawmode();
	rows = 24;
	columns = 80;
	IF_FEATURE_VI_ASK_TERMINAL(T.get_rowcol_error =) query_screen_dimensions();
#if ENABLE_FEATURE_VI_ASK_TERMINAL
	if (T.get_rowcol_error /* TODO? && no input on stdin */) {
		uint64_t k;
		write1(ESC"[999;999H" ESC"[6n");
		fflush_all();
		k = safe_read_key(/*timeout_ms:*/ 100);
		if ((int32_t)k == KEYCODE_CURSOR_POS) {
			uint32_t rc = (k >> 32);
			columns = (rc & 0x7fff);
			if (columns > MAX_SCR_COLS)
				columns = MAX_SCR_COLS;
			rows = ((rc >> 16) & 0x7fff);
			if (rows > MAX_SCR_ROWS)
				rows = MAX_SCR_ROWS;
		}
	}
#endif
}

void FAST_FUNC alternate_screen_buffer_start(void)
{
	// "Save cursor, use alternate screen buffer, clear screen"
	write1(ESC"[?1049h");
}

void FAST_FUNC alternate_screen_buffer_end(void)
{
	// "Use normal screen buffer, restore cursor"
	write1(ESC"[?1049l");
}
